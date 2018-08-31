#include "server_session.h"
#include <common/server_client_send.h>
#include <common/async_net.h>
#include <common/serialize_filesystem.h>
#include <fstream>
#include <limits>
using namespace cnc;
using namespace cnc::client::server;
using common::server::client::protocol;
using common::server::client::send;
using boost::asio::ip::tcp;

session::session(boost::asio::io_context &context)
	: m_socket(context)
{

}

session::~session()
{
	close();
}

void session::close()
{
	if (m_closed || !m_connected)
		return;

	m_socket.shutdown(tcp::socket::shutdown_both);
	m_socket.close();
	m_closed = true;
}

common::task<void> session::connect(tcp::endpoint endpoint, const common::mac_addr &addr)
{
	if (m_connecting)
		throw std::runtime_error("already connecting");
	else if (m_connected)
		throw std::runtime_error("already connected");

	m_connecting = true;
	co_await common::async_connect(m_socket, endpoint);
	auto result = co_await send::hello(m_socket, { addr });
	if (result.err)
		throw std::runtime_error(result.err_msg);

	m_connecting = false;
	m_connected = true;
}

common::task<std::optional<std::string>> session::listen()
{
	if (!m_connected)
		throw std::runtime_error("not connected");
	else if (m_listening)
		throw std::runtime_error("already listening");
	else if (closed())
		throw std::runtime_error("session closed");

	m_listening = true;
	while (m_listening)
	{
		using types = protocol::types;
		auto header = co_await send::session::recv_header(m_socket);
		switch (header.get_type())
		{
			case types::RECV_FILE:
			{
				auto msg = co_await send::session::recv_msg(m_socket, header.get_payload_size());
				auto path = common::deserialize<std::filesystem::path>(msg);
				std::filesystem::create_directories(path.parent_path());
				std::ofstream file(path.native(), std::ios::out | std::ios::binary);
				if (!file)
				{
					co_await send::session::send_msg(m_socket, types::ERR, "unable to open file");
					break;
				}

				co_await send::session::send_msg(m_socket, types::OK);
				
				header = co_await send::session::recv_header(m_socket);
				if (header.get_type() != types::BLOB)
					throw std::runtime_error("BLOB expected");
					//throw unexpected_message_error(*this, header, "BLOB expected");

				co_await send::session::recv_stream(m_socket, file, header.get_payload_size());
				break;
			}
			case types::SEND_FILE:
			{
				auto msg = co_await send::session::recv_msg(m_socket, header.get_payload_size());
				auto path = common::deserialize<std::filesystem::path>(msg);
				std::ifstream file(path.native(), std::ios::in | std::ios::binary);
				if (!file)
				{
					co_await send::session::send_msg(m_socket, types::ERR, "unable to open file");
					break;
				}
				
				co_await send::session::send_msg(m_socket, types::OK);
				auto size = std::filesystem::file_size(path);
				if (size > std::numeric_limits<send::session::size_type>::max())
					throw std::range_error("file too big");

				co_await send::session::send_stream(m_socket, types::BLOB, file, static_cast<send::session::size_type>(size));
				break;
			}
			case types::QUIT:
			{
				auto msg = co_await send::session::recv_msg(m_socket, header.get_payload_size());
				m_listening = false;
				m_connected = false;
				close();
				co_return msg;
			}
		}
	}

	m_connected = false;
	if (!closed())
	{
		co_await send::quit(m_socket, "session stopped");
		close();
	}

	m_listening = false;
	co_return std::optional<std::string>{};
}

void session::stop() noexcept
{
	m_listening = false;
}