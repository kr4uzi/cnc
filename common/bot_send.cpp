#include "bot_send.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"

using namespace cnc::common;

bot_send::awaitable_type<bot_send::err_or_empty_ok_result> bot_send::recv_err_or_empty_ok(bot_send::socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != bot_protocol::magic_byte)
		throw std::runtime_error("unexpected magic byte");

	using types = bot_protocol::types;
	switch (response.type())
	{
	case types::OK:
		if (response.payload_size() != 0)
			throw std::runtime_error("unexpected_message_error");

		co_return err_or_empty_ok_result{ false };

	case types::ERR:
	{
		auto msg = co_await session_type::recv_msg(socket, response.payload_size());
		co_return err_or_empty_ok_result{ true, msg };
	}
	}

	throw std::runtime_error("unexpected_message_error");
}

bot_send::awaitable_type<bot_send::err_or_ok_result> bot_send::recv_err_or_ok(bot_send::socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != bot_protocol::magic_byte)
		throw std::runtime_error("unexpected magic byte");

	using types = header_type::message_type;
	switch (response.type())
	{
	case types::OK:
	{
		auto msg = co_await session_type::recv_msg(socket, response.payload_size());
		co_return err_or_ok_result{ false, "", msg };
	}
	case types::ERR:
	{
		auto msg = co_await session_type::recv_msg(socket, response.payload_size());
		co_return err_or_ok_result{ true, msg };
	}
	}

	throw std::runtime_error("unexpected_message_error");
}

bot_send::awaitable_type<bot_send::hello_result> bot_send::hello(bot_send::socket_type &socket, const bot_protocol::hello_data &data)
{
	co_await session_type::send_msg(socket, header_type::message_type::HELLO, serialize(data));
	auto result = co_await recv_err_or_empty_ok(socket);
	co_return result;
}

bot_send::awaitable_type<bot_send::connect_result> bot_send::connect(bot_send::socket_type &socket, const bot_protocol::connect_data &data)
{
	co_await session_type::send_msg(socket, header_type::message_type::CONNECT, serialize(data));
	auto result = co_await recv_err_or_empty_ok(socket);
	co_return result;
}

bot_send::awaitable_type<bot_send::recv_file_result> bot_send::recv_file(bot_send::socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size)
{
	co_await session_type::send_msg(socket, header_type::message_type::RECV_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket);
	if (result.err)
		return recv_file_result{ true, result.err_msg };

	co_await session_type::send_stream(socket, header_type::message_type::BLOB, in, size);
	co_return recv_file_result{ false };
}

bot_send::awaitable_type<bot_send::send_file_result> bot_send::send_file(bot_send::socket_type &socket, const std::filesystem::path &path, std::ostream &out)
{
	co_await session_type::send_msg(socket, header_type::message_type::SEND_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket);
	if (result.err)
		return send_file_result{ true, result.err_msg };
	
	auto response = co_await session_type::recv_header(socket);
	if (response.type() != header_type::message_type::BLOB)
		throw std::runtime_error("unexpected_message_error");

	co_await session_type::recv_stream(socket, out, response.payload_size());
	co_return send_file_result{ false };
}

bot_send::awaitable_type<bot_send::quit_result> bot_send::quit(bot_send::socket_type &socket)
{
	auto result = co_await quit(socket, "");
	co_return result;
}

bot_send::awaitable_type<bot_send::quit_result> bot_send::quit(bot_send::socket_type &socket, const std::string &msg)
{
	co_await session_type::send_msg(socket, bot_protocol::types::QUIT, msg);
	auto result = co_await recv_err_or_empty_ok(socket);
	co_return result;
}
