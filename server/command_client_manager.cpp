#include "command_client_manager.h"
#include "../common/async_net.h"
using namespace cnc;
using namespace cnc::server;
using boost::asio::ip::tcp;
using boost::beast::websocket::close_code;
using cnc::common::server::observer::protocol;

command_client_manager::command_client_manager(boost::asio::io_context &context)
	: m_context(context), m_acceptor(context, tcp::endpoint(tcp::v4(), protocol::tcp_port))
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
		auto &sess = m_potentials.emplace_back(std::move(socket));
		auto sess_iter = --m_potentials.end();

		auto task = [this, &sess, sess_iter]() -> common::task<void>
		{
			try
			{
				co_await sess.initialize();

				auto &client = m_clients.emplace_back(std::move(sess));
				m_potentials.erase(sess_iter);

				auto client_iter = --m_clients.end();
				try
				{
					co_await client.run();
				}
				catch (...)
				{

				}

				m_clients.erase(client_iter);
			}
			catch (...)
			{
				m_potentials.erase(sess_iter);
			}
		}();

		tasks.push_back(std::move(task));
	}

	for (auto &task : tasks)
		co_await task;
}

void command_client_manager::stop()
{
	m_running = false;
}