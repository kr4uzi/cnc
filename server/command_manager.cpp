#include "command_manager.h"
#include "../common/basic_session.h"
#include "../common/server_observer_protocol.h"
#include <boost/beast/core.hpp>
#include <json_spirit/json_spirit.h>
#include <utility>
#include <type_traits>
#include <stdexcept>
#include <sstream>
#include <map>
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using namespace boost::beast::websocket;
using namespace cnc::server;
using namespace cnc::common::server;
using observer::protocol;
using types = protocol::types;

types type_from_uint(unsigned int value)
{
	auto type = static_cast<types>(value);
	if (type < types::LAST_MEMBER_UNUSED && type > types::FIRST_MEMBER_UNUSED)
		return type;

	throw std::out_of_range("invalid observer::protocol type");
}

std::map<std::underlying_type<types>::type, std::string> types_to_string()
{
	std::invoke_result<decltype(types_to_string)>::type result;
	auto first = static_cast<std::underlying_type<types>::type>(types::FIRST_MEMBER_UNUSED);
	auto last = static_cast<std::underlying_type<types>::type>(types::LAST_MEMBER_UNUSED);

	for (auto i = first + 1; i < last; i++)
	{
		std::ostringstream ss;
		ss << static_cast<types>(i);
		result.emplace(i, ss.str());
	}

	return result;
}

command_session::command_session(tcp::socket socket)
	: m_socket(std::move(socket))
{

}

void command_session::stop()
{
	m_running = false;
}

void command_session::run(yield_context yield)
{
	m_running = true;
	m_socket.async_accept(yield);

	while (m_running)
	{
		try
		{
			boost::beast::multi_buffer buffer;
			m_socket.async_read(buffer, yield);
			auto msg = boost::beast::buffers_to_string(buffer);
			json_spirit::Value json;
			json_spirit::read(msg, json);

			auto raw_type = json.get("type", static_cast<std::underlying_type<types>::type>(types::FIRST_MEMBER_UNUSED)).asUInt();
			auto type = static_cast<types>(raw_type);
			switch (type)
			{
			case types::HELLO:
			{
				if (m_initialized)
					throw session_error("already initialized");

				m_initialized = true;
				m_socket.text(true);

				json::::Value response;
				response["type"] = static_cast<std::underlying_type<types>::type>(types::OK);
				m_socket.async_write(Json::FastWriter().write(response), yield);
				break;
			}
			case types::OBSERVE:
				break;
			case types::UNOBSERVE:
				break;
			default:
				throw session_error("unknown type");
			}
		}
		catch (std::exception &e)
		{

		}
	}
}