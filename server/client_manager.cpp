#include "client_manager.h"
#include "client.h"
#include "py_manager.h"
#include "converters.h"
#include <common/server_client_protocol.h>
#include <common/async_net.h>
#include <iostream>
#include <regex>
using namespace cnc::server;
using namespace cnc::common;
using boost::asio::ip::tcp;
using cnc::common::server::client::protocol;
using types = protocol::types;

client_manager::client_manager(boost::asio::io_context &context)
	: m_context(context), m_acceptor(context, tcp::endpoint(tcp::v4(), protocol::tcp_port))
{

}

client_manager::~client_manager()
{
	stop();
}

void client_manager::stop()
{
	if (!m_running || m_stopping)
		return;

	m_stopping = true;
	for (auto &client : m_clients)
		client.stop();
}

std::future<void> client_manager::run()
{
	// this should throw because the the caller might assume that the returned task is shared with a potential
	// previous "run"
	if (m_running)
		throw std::runtime_error("already running");

	std::vector<common::task<void>> tasks;
	m_running = true;

	std::cout << "client manager listening on port " << m_acceptor.local_endpoint().port() << "\n";

	while (!m_stopping)
	{
		try
		{
			auto socket = co_await common::async_acccept(m_acceptor);
			auto task = [this, &tasks, socket{ std::move(socket) }]() mutable->common::task<void>
			{
				try
				{
					potential_client potential{ std::move(socket) };
					auto data = co_await potential.recv_hello();
					for (auto &session : m_clients)
					{
						if (data.mac == session.mac())
						{
							std::cerr << '[' << common::to_string(data.mac) << "][ERR] tried to connect, but mac already connected\n";
							co_await potential.reject("mac already registered");
							// jump to cleanup
							throw;
						}
					}

					co_await potential.accept();
					auto client = m_clients.emplace(m_clients.end(), std::move(potential), std::move(data));
					std::cout << '[' << client->mac() << "] connected\n";
					py_manager::instance().handle("ClientConnect", client->mac(), client->data());

					try
					{
						co_await client->run();
					}
					catch (std::exception &e)
					{
						std::cerr << '[' << common::to_string(client->mac()) << "][ERR] " << e.what() << '\n';
					}
					catch (...)
					{
						std::cerr << '[' << common::to_string(client->mac()) << "][ERR] unknown exception occurred\n";
					}


					std::cout << '[' << client->mac() << "] disconnected\n";
					py_manager::instance().handle("ClientDisconnect", client->mac());
					m_clients.erase(client);
				}
				catch (...)
				{

				}

				// this doesnt remove this coroutine from "tasks", but the next coroutine will do so
				// this ensures that there aren't too many (acutally only one) "dead" coroutines in the tasks vector
				if (m_running)
				{
					for (auto i = tasks.begin(); i != tasks.end();)
					{
						if (i->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
							i = tasks.erase(i);
						else
							++i;
					}
				}
			}();

			tasks.push_back(std::move(task));
		}
		catch (...)
		{
			if (m_stopping)
				break;

			throw;
		}
	}

	for (auto &task : tasks)
		co_await task;

	tasks.clear();

	std::cout << "client manager stopped\n";

	m_running = false;
	m_stopping = false;
}

std::vector<client_data> client_manager::data() const
{
	std::vector<client_data> datas;
	for (auto &client : m_clients)
		datas.push_back({ client.ip(), client.mac() });

	return datas;
}