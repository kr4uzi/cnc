#include "command_client.h"
#include <cstdint>
#include <boost/beast/core.hpp>
#include <boost/asio/use_future.hpp>
#include <json_spirit/json_spirit.h>
#include <utility>
#include <stdexcept>
using boost::asio::ip::tcp;
using namespace boost::beast::websocket;
using namespace cnc;
using namespace cnc::server;
using namespace cnc::common::server;
using observer::protocol;
using types = protocol::types;

types type_from_uint(std::uint64_t value)
{
	auto type = static_cast<types>(value);
	if (type < types::LAST_MEMBER_UNUSED && type > types::FIRST_MEMBER_UNUSED)
		return type;

	throw std::out_of_range("invalid observer::protocol type");
}

potential_command_client::potential_command_client(tcp::socket socket)
	: m_socket(std::move(socket))
{

}

potential_command_client::~potential_command_client()
{

}

common::task<void> potential_command_client::close(boost::beast::websocket::close_code reason)
{
	if (m_closed || m_closing)
		return;

	m_closing = true;
	co_await m_socket.async_close(reason, boost::asio::use_future);
	m_closing = false;
	m_closed = true;
}

common::task<websocket_message> potential_command_client::recv_msg()
{
	boost::beast::multi_buffer buffer;
	co_await m_socket.async_read(buffer, boost::asio::use_future);
	auto msg = boost::beast::buffers_to_string(buffer.data());
	json_spirit::Value json;
	json_spirit::read(msg, json);

	auto type = json.get("type", static_cast<std::underlying_type<types>::type>(types::FIRST_MEMBER_UNUSED)).getUInt64();
	co_return websocket_message{ type_from_uint(type), json };
}

common::task<void> potential_command_client::initialize()
{
	if (!m_initialized && m_initializing)
		throw std::runtime_error("initialize previously failed");
	else if (m_initialized || m_initializing)
		return;

	m_initializing = true;
	co_await m_socket.async_accept(boost::asio::use_future);

	auto msg = co_await recv_msg();
	if (msg.type != types::HELLO)
		throw invalid_message_error(*this, msg, "HELLO expected");

	m_initialized = true;
	m_initializing = false;
}

command_client::command_client(potential_command_client client)
	: potential_command_client(std::move(client))
{

}

void command_client::stop()
{
	m_running = false;
}

common::task<void> command_client::run()
{
	if (m_running)
		return;

	m_running = true;
	co_await m_socket.async_accept(boost::asio::use_future);

	while (m_running)
	{
		auto msg = co_await recv_msg();
		switch (msg.type)
		{
		case types::OBSERVE:
			break;
		case types::UNOBSERVE:
			break;
		default:
			throw invalid_message_error(*this, msg, "unhandled type");
		}
	}
}