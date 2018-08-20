#include "command_client_manager.h"
#include "command_client.h"
#include <common/async_net.h>
#include <iostream>
using namespace cnc;
using namespace cnc::server;
using boost::asio::ip::tcp;
using boost::beast::websocket::close_code;
using cnc::common::server::observer::protocol;

singleton<command_client_manager> *singleton<command_client_manager>::m_instance;
command_client_manager::command_client_manager(boost::asio::io_context &context)
	: m_context(context), m_acceptor(context, tcp::endpoint(tcp::v4(), protocol::tcp_port))
{

}

command_client_manager::~command_client_manager()
{

}

common::task<void> command_client_manager::run()
{
	if (m_running)
		return;

	std::vector<common::task<void>> tasks;
	m_running = true;

	while (m_running)
	{
		auto socket = co_await common::async_acccept(m_acceptor);
		auto task = [this, &tasks, socket{ std::move(socket) }]() mutable ->common::task<void>
		{
			try
			{
				potential_command_client potential{ std::move(socket) };
				co_await potential.initialize();

				auto client = m_clients.emplace(m_clients.end(), std::move(potential));
				std::cout << "[cmdmgr][" << client->ip() << "] connected\n";

				try
				{
					co_await client->run();
				}
				catch (std::exception &e)
				{
					std::cerr << "[cmdmgr][" << client->ip() << "][ERR] " << e.what() << '\n';
				}
				catch (...)
				{
					std::cerr << "[cmdmgr][" << client->ip() << "][ERR] unknown exception occurred\n";
				}

				std::cout << "[cmdmgr][" << client->ip() << "] disconnected\n";
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

	// at this point m_running == false, so the tasks do not invalidate the iterators of tasks
	for (auto &task : tasks)
		co_await task;

	tasks.clear();
}

void command_client_manager::stop()
{
	m_running = false;
}