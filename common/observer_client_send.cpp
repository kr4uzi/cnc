#include "observer_client_send.h"
#include "serialize_filesystem.h"
#include "session.h"

using namespace cnc::common;
using namespace cnc::common::observer::client;
using types = protocol::types;
using boost::asio::ip::tcp;

task<send::hello_result> send::hello(tcp::socket &socket)
{
	auto result = co_await send::hello(socket, "");
	co_return result;
}

task<send::hello_result> send::hello(tcp::socket &socket, const std::string &msg)
{
	co_await session::send_msg(socket, types::HELLO, msg);
	auto result = co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
	co_return result;
	co_return hello_result{false};
}

task<send::create_directory_result> send::create_directory(tcp::socket &socket, const std::filesystem::path &path)
{
	co_await session::send_msg(socket, types::CREATE_DIRECTORY, path.string());
	co_return co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
}

task<send::list_directory_result> send::list_directory(tcp::socket &socket, const std::filesystem::path &path)
{
	co_await session::send_msg(socket, types::LIST_DIRECTORY, path.string());
	auto response = co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
	if (response.err)
		co_return list_directory_result{ false, response.err_msg };

	co_return list_directory_result{ false, "", common::deserialize<protocol::directory_view>(response.msg) };
}

task<send::recv_file_result> send::recv_file(tcp::socket &socket, const std::filesystem::path &path, std::istream &in, protocol::header::size_type size)
{
	co_await session::send_msg(socket, types::RECV_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
	if (result.err)
		co_return recv_file_result{ true, result.err_msg };

	co_await session::send_stream(socket, types::BLOB, in, size);
	co_return recv_file_result{ false };
}

task<send::send_file_result> send::send_file(tcp::socket &socket, const std::filesystem::path &path, std::ostream &out)
{
	co_await session::send_msg(socket, types::SEND_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
	if (result.err)
		co_return send_file_result{ true, result.err_msg };

	auto response = co_await session::recv_header(socket);
	if (response.get_type() != types::BLOB)
		throw std::runtime_error("unexpected_message_error");
		//throw session::unexpected_message_error(response);

	co_await session::recv_stream(socket, out, response.get_payload_size());
	co_return send_file_result{ false };
}

task<send::execute_result> send::execute(tcp::socket &socket, const std::string &cmd)
{
	co_await session::send_msg(socket, types::EXECUTE, cmd);
	auto result = co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
	co_return execute_result{ result.err, result.err_msg, result.msg };
}

task<send::quit_result> send::quit(tcp::socket &socket)
{
	co_return co_await quit(socket, "");
}

task<send::quit_result> send::quit(tcp::socket &socket, const std::string &msg)
{
	co_await session::send_msg(socket, types::QUIT, msg);
	co_return co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
}

task<send::err_or_empty_ok_result> send::recv_err_or_empty_ok(tcp::socket &socket, const protocol::header &response)
{
	switch (response.get_type())
	{
	case types::OK:
		if (response.get_payload_size() != 0)
			throw std::runtime_error("unexpected_message_error");
			//throw session::unexpected_message_error(response, "no payload expected");

		co_return err_or_empty_ok_result{ false };

	case types::ERR:
		co_return err_or_empty_ok_result{ true, co_await session::recv_msg(socket, response.get_payload_size()) };
	}

	throw std::runtime_error("unexpected_message_error");
	//throw session::unexpected_message_error(response);
}

task<send::err_or_ok_result> send::recv_err_or_ok(tcp::socket &socket, const protocol::header &response)
{
	switch (response.get_type())
	{
	case types::OK:
	{
		auto msg = co_await session::recv_msg(socket, response.get_payload_size());
		co_return err_or_ok_result{ false, "", msg };
	}
	case types::ERR:
	{
		auto msg = co_await session::recv_msg(socket, response.get_payload_size());
		co_return err_or_ok_result{ true, msg };
	}
	}

	throw std::runtime_error("unexpected_message_error");
	//throw session::unexpected_message_error(response);
}