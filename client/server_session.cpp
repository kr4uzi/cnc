#include "server_session.h"
#include "../common/async_net.h"
#include "../common/serialize_filesystem.h"
#include <fstream>
using namespace cnc;
using namespace cnc::client::server;
using boost::asio::ip::tcp;

session::session(boost::asio::io_context &context)
	: common::server::client::session(tcp::socket(context))
{

}

common::task<void> session::connect(tcp::endpoint endpoint, const common::mac_addr &addr)
{
	if (m_connecting)
		throw std::runtime_error("already connecting");
	else if (m_connected)
		throw std::runtime_error("already connected");

	m_connecting = true;
	co_await common::async_connect(m_socket, endpoint);
	auto result = co_await hello({ addr });
	if (result.err)
		throw std::runtime_error(result.err_msg);

	m_connecting = false;
	m_connected = true;
}

common::task<std::optional<std::string>> session::listen()
{
	if (!m_connected)
		throw std::runtime_error("not connected");
	else if (m_running)
		throw std::runtime_error("already running");

	m_running = true;
	while (m_running)
	{
		using types = protocol::types;
		auto header = co_await recv_header();
		switch (header.get_type())
		{
			case types::RECV_FILE:
			{
				auto msg = co_await recv_msg(header.get_payload_size());
				auto path = common::deserialize<std::filesystem::path>(msg);
				std::filesystem::create_directories(path.parent_path());
				std::ofstream file(path.native(), std::ios::out | std::ios::binary);
				if (!file)
				{
					co_await send_msg(types::ERR, "unable to open file");
					break;
				}

				co_await send_msg(types::OK);
				
				header = co_await recv_header();
				if (header.get_type() != types::BLOB)
					throw std::runtime_error("BLOB expected");
					//throw unexpected_message_error(*this, header, "BLOB expected");

				co_await recv_stream(file, header.get_payload_size());
				break;
			}
			case types::SEND_FILE:
			{
				auto msg = co_await recv_msg(header.get_payload_size());
				auto path = common::deserialize<std::filesystem::path>(msg);
				std::ifstream file(path.native(), std::ios::in | std::ios::binary);
				if (!file)
				{
					co_await send_msg(types::ERR, "unable to open file");
					break;
				}
				
				co_await send_msg(types::OK);				
				co_await send_stream(types::BLOB, file, std::filesystem::file_size(path));
				break;
			}
			case types::QUIT:
			{
				auto msg = co_await recv_msg(header.get_payload_size());
				m_running = false;
				m_connected = false;
				close();
				co_return msg;
			}
		}
	}

	m_connected = false;
	if (!closed())
	{
		co_await quit("session stopped");
		close();
	}

	m_running = false;
	co_return std::optional<std::string>{};
}

void session::stop() noexcept
{
	m_running = false;
}