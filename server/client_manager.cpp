#include "client_manager.h"
#include "client.h"
#include <common/server_client_protocol.h>
#include <iostream>
#include <pybind11/embed.h>
using namespace cnc::server;
using namespace cnc::common;
namespace py = pybind11;
using boost::asio::ip::tcp;
using cnc::common::server::client::protocol;
using types = protocol::types;

namespace cnc { namespace server {

} }

singleton<client_manager> *singleton<client_manager>::m_instance;
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
	if (!m_running)
		return;

	m_running = false;
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

	while (m_running)
	{
		auto socket = co_await common::async_acccept(m_acceptor);
		auto task = [this, &tasks, socket{ std::move(socket) }]() mutable ->common::task<void>
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
						co_return;
					}
				}

				co_await potential.accept();
				auto client = m_clients.emplace(m_clients.end(), std::move(potential), std::move(data));
				std::cout << '[' << client->mac() << "] connected\n";

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

	for (auto &task : tasks)
		co_await task;

	tasks.clear();
}