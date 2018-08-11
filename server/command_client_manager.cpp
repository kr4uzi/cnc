#include "command_client_manager.h"
using namespace cnc::server;
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using boost::beast::websocket::close_code;
using cnc::common::server::observer::protocol;

command_client_manager::command_client_manager(boost::asio::io_context &context)
	: m_context(context), m_acceptor(context, tcp::endpoint(tcp::v4(), protocol::tcp_port))
{

}

void command_client_manager::run(boost::asio::yield_context yield)
{
	if (m_running)
		return;

	m_running = true;
	accept(yield);
}

void command_client_manager::stop(yield_context yield)
{

}

void command_client_manager::accept(yield_context yield)
{
	while (m_running)
	{
		try
		{
			tcp::socket socket(m_context);
			m_acceptor.async_accept(socket, yield);
			auto &sess = m_potentials.emplace_back(std::move(socket));
			auto iter = --m_potentials.end();
			auto close_conn = sess.on_close.connect([this, iter]
			{
				m_potentials.erase(iter);
			});
			auto err_conn = sess.on_error.connect([this, &sess](auto ptr)
			{
				boost::asio::spawn(m_context, std::bind(&potential_command_client::close, std::ref(sess), close_code::internal_error, std::placeholders::_1));
			});

			boost::asio::spawn(m_context, [this, &sess, close_conn, err_conn, iter](auto yield)
			{
				sess.initialize(yield);
				close_conn.disconnect();
				err_conn.disconnect();
				
				auto &client = m_clients.emplace_back(std::move(sess));
				m_potentials.erase(iter);
				
				auto new_iter = --m_clients.end();
				client.on_close.connect([this, new_iter]
				{
					m_clients.erase(new_iter);
				});
				client.on_error.connect([this, &client](auto ptr)
				{
					client.stop();
					boost::asio::spawn(m_context, std::bind(&command_client::close, std::ref(client), close_code::internal_error, std::placeholders::_1));
					on_error(ptr);
				});
			});
		}
		catch (...)
		{
			on_error(std::current_exception());
		}
	}
}