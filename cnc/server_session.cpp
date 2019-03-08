#include "server_session.h"
using namespace cnc;

server_session::server_session(boost::asio::io_context &context, const boost::asio::ip::address &address, unsigned short port)
	: m_socket(context), m_address(address), m_port(port)
{

}

server_session::~server_session()
{

}

void server_session::stop()
{
	boost::system::error_code ec;
	m_socket.shutdown(decltype(m_socket)::shutdown_both, ec);
	m_socket.close(ec);
}

server_session::awaitable_type<void> server_session::run()
{
	auto token = co_await boost::asio::experimental::this_coro::token();
	co_await m_socket.async_connect({ m_address, m_port }, token);
	auto hello_result = co_await common::cmd_send::hello(m_socket);
	on_hello(hello_result.clients);

	while (!m_stopped)
	{
		using protocol = common::cmd_protocol;
		using session = common::cmd_send::session_type;
		using types = protocol::types;

		for (auto &task : m_tasks)
			co_await task;
		m_tasks.clear();

		auto header = co_await session::recv_header(m_socket);		
		switch (header.type())
		{
		case types::REGISTERED:
		{
			auto msg = co_await session::recv_msg(m_socket, header.payload_size());
			auto data = common::deserialize<protocol::client_data>(msg);
			on_registered(data);
			break;
		}
		case types::UNREGISTERED:
		{
			auto msg = co_await session::recv_msg(m_socket, header.payload_size());
			auto mac = common::deserialize<common::mac_addr>(msg);
			on_unregistered(mac);
			break;
		}
		case types::LOG:
		{
			auto msg = co_await session::recv_msg(m_socket, header.payload_size());
			auto log = common::deserialize<protocol::log>(msg);
			on_log(log);
			break;
		}
		case types::QUIT:
		{
			std::string msg;
			if (header.payload_size())
				msg = co_await session::recv_msg(m_socket, header.payload_size());
			on_quit(msg);
			m_stopped = true;
			break;
		}
		default:
			throw std::runtime_error("unexpected packet received");
		}
	}
}