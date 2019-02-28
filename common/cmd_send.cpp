#include "cmd_send.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"

using namespace cnc::common;

cmd_send::awaitable_type<cmd_send::err_or_empty_ok_result> cmd_send::recv_err_or_empty_ok(socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != cmd_protocol::magic_byte)
		throw std::runtime_error("invalid magic byte received");

	using types = cmd_protocol::types;
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

cmd_send::awaitable_type<cmd_send::err_or_ok_result> cmd_send::recv_err_or_ok(socket_type &socket)
{
	auto response = co_await session_type::recv_header(socket);
	if (response.magic_byte() != cmd_protocol::magic_byte)
		throw std::runtime_error("invalid magic byte received");

	using types = cmd_protocol::types;
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

cmd_send::awaitable_type<cmd_send::observe_result> cmd_send::observe(socket_type &socket, const mac_addr &mac)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::OBSERVE, to_string(mac));
	auto response = co_await recv_err_or_ok(socket);
	if (response.err)
		co_return observe_result{ true, response.err_msg };

	co_return observe_result{ false, "", deserialize<cmd_protocol::logs>(response.msg) };
}

cmd_send::awaitable_type<cmd_send::hello_result> cmd_send::hello(socket_type &socket)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::OBSERVE);
	auto response = co_await recv_err_or_ok(socket);
	if (response.err)
		co_return hello_result{ true, response.err_msg };

	co_return hello_result{ false, "", deserialize<cmd_protocol::clients>(response.msg) };
}

cmd_send::awaitable_type<cmd_send::recv_file_result> cmd_send::recv_file(socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::RECV_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket);
	if (result.err)
		co_return recv_file_result{ true, result.err_msg };

	co_await session_type::send_stream(socket, cmd_protocol::types::BLOB, in, size);
	co_return recv_file_result{ false };
}

cmd_send::awaitable_type<cmd_send::send_file_result> cmd_send::send_file(socket_type &socket, const std::filesystem::path &path, std::ostream &out)
{
	using types = cmd_protocol::types;
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

cmd_send::awaitable_type<cmd_send::unobserve_result> cmd_send::quit(socket_type &socket)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::QUIT);
	co_return co_await recv_err_or_empty_ok(socket);
}

cmd_send::awaitable_type<cmd_send::unobserve_result> cmd_send::quit(socket_type &socket, const std::string &message)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::QUIT, message);
	co_return co_await recv_err_or_empty_ok(socket);
}

cmd_send::awaitable_type<cmd_send::unobserve_result> cmd_send::unobserve(socket_type &socket, const cnc::common::mac_addr &mac)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::UNOBSERVE, to_string(mac));
	co_return co_await recv_err_or_empty_ok(socket);
}

cmd_send::awaitable_type<cmd_send::connect_result> cmd_send::connect(socket_type &socket, const cmd_protocol::client_data &data)
{
	co_await session_type::send_msg(socket, cmd_protocol::types::CONNECT, serialize(data));
	co_return co_await recv_err_or_empty_ok(socket);
}