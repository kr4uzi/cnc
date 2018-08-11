#include "client_session.h"
#include "serialize_filesystem.h"

using namespace cnc::common;
using namespace cnc::common::client;
using types = protocol::types;

session::session(boost::asio::ip::tcp::socket socket)
	: basic_session<protocol>(std::move(socket))
{

}

std::future<session::hello_result> session::hello()
{
	co_return co_await hello("");
}

std::future<session::hello_result> session::hello(const std::string &msg)
{
	co_await send_msg(types::HELLO, msg);
	co_return co_await recv_err_or_ok(co_await recv_header());
}

std::future<session::create_directory_result> session::create_directory(const std::filesystem::path &path)
{
	co_await send_msg(types::CREATE_DIRECTORY, path.string());
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

std::future<session::list_directory_result> session::list_directory(const std::filesystem::path &path)
{
	co_await send_msg(types::LIST_DIRECTORY, path.string());
	auto response = co_await recv_header();
	switch (response.get_type())
	{
	case types::OK:
		break;
	case types::ERR:
		co_return list_directory_result{ true, co_await recv_msg(response.get_payload_size()) };
	default:
		throw unexpected_message_error(*this, response);
	}

	auto msg = co_await recv_msg(response.get_payload_size());
	co_return list_directory_result{ false, "", protocol::directory_view_from_string(msg) };
}

std::future<session::recv_file_result> session::recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size)
{
	co_await send_msg(types::RECV_FILE, to_string(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		co_return recv_file_result{ true, result.err_msg };

	co_await send_stream(types::BLOB, in, size);
	co_return recv_file_result{ false };
}

std::future<session::send_file_result> session::send_file(const std::filesystem::path &path, std::ostream &out)
{
	co_await send_msg(types::SEND_FILE, to_string(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		co_return send_file_result{ true, result.err_msg };

	auto response = co_await recv_header();
	switch (response.get_type())
	{
	case types::BLOB:
		break;
	default:
		throw unexpected_message_error(*this, response);
	}

	co_await recv_stream(out, response.get_payload_size());
	co_return send_file_result{ false };
}

std::future<session::execute_result> session::execute(const std::string &cmd)
{
	co_await send_msg(types::EXECUTE, cmd);
	auto result = co_await recv_err_or_ok(co_await recv_header());
	co_return execute_result{ result.err, result.err_msg, result.msg };
}

std::future<session::quit_result> session::quit()
{
	co_return co_await quit("");
}

std::future<session::quit_result> session::quit(const std::string &msg)
{
	co_await send_msg(types::QUIT, msg);
	co_return co_await recv_err_or_ok(co_await recv_header());
}

std::future<session::err_or_empty_ok_result> session::recv_err_or_empty_ok(const protocol::header &response)
{
	switch (response.get_type())
	{
	case types::OK:
		if (response.get_payload_size() != 0)
			throw unexpected_message_error(*this, response, "no payload expected");

		co_return err_or_empty_ok_result{ false };

	case types::ERR:
		co_return err_or_empty_ok_result{ true, co_await recv_msg(response.get_payload_size()) };
	}

	throw unexpected_message_error(*this, response);
}

std::future<session::err_or_ok_result> session::recv_err_or_ok(const protocol::header &response)
{	
	switch (response.get_type())
	{
	case types::OK:
	{
		auto msg = co_await recv_msg(response.get_payload_size());
		co_return err_or_ok_result{ false, "", msg };
	}
	case types::ERR:
	{
		auto msg = co_await recv_msg(response.get_payload_size());
		co_return err_or_ok_result{ true, msg };
	}
	}

	throw unexpected_message_error(*this, response);
}