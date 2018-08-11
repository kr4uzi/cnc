#include "command_client.h"
#include <cstdint>
#include <boost/beast/core.hpp>
#include <json_spirit/json_spirit.h>
#include <utility>
#include <stdexcept>
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using namespace boost::beast::websocket;
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

void potential_command_client::close(boost::beast::websocket::close_code reason, boost::asio::yield_context yield)
{
	if (m_closed)
		return;

	m_closing = true;
	m_socket.async_close(reason, yield);
	m_closing = false;
	m_closed = true;
	on_close();
}

websocket_message potential_command_client::get_message(yield_context yield)
{
	boost::beast::multi_buffer buffer;
	m_socket.async_read(buffer, yield);
	auto msg = boost::beast::buffers_to_string(buffer.data());
	json_spirit::Value json;
	json_spirit::read(msg, json);

	auto type = json.get("type", static_cast<std::underlying_type<types>::type>(types::FIRST_MEMBER_UNUSED)).getUInt64();
	return { type_from_uint(type), json };
}

void potential_command_client::initialize(yield_context yield)
{
	if (!m_initialized && m_initializing)
		throw std::runtime_error("initialize previously failed");
	else if (m_initialized || m_initializing)
		return;

	m_initializing = true;
	m_socket.async_accept(yield);

	try
	{
		auto msg = get_message(yield);
		if (msg.type != types::HELLO)
			throw invalid_message_error(*this, msg, "HELLO expected");

		m_initialized = true;
		m_initializing = false;
	}
	catch (...)
	{
		on_error(std::current_exception());
	}
}

command_client::command_client(potential_command_client client)
	: potential_command_client(std::move(client))
{

}

void command_client::stop()
{
	m_running = false;
}

void command_client::run(yield_context yield)
{
	m_running = true;
	m_socket.async_accept(yield);

	while (m_running)
	{
		try
		{
			auto msg = get_message(yield);
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
		catch (...)
		{
			on_error(std::current_exception());
		}
	}
}