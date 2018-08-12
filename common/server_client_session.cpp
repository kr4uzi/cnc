#include "server_client_session.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"
using boost::asio::ip::tcp;
using namespace cnc::common;
using namespace cnc::common::server::client;
using types = protocol::types;

session::session(tcp::socket socket)
	: basic_session<protocol>(std::move(socket))
{

}

task<session::err_or_empty_ok_result> session::recv_err_or_empty_ok(const protocol::header &response)
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

task<session::err_or_ok_result> session::recv_err_or_ok(const protocol::header &response)
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

task<session::hello_result> session::hello(const protocol::hello_data &data)
{
	co_await send_msg(types::HELLO, protocol::to_string(data));
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

task<session::connect_result> session::connect(const protocol::connect_data &data)
{
	co_await send_msg(types::CONNECT, protocol::to_string(data));
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

task<session::recv_file_result> session::recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size)
{
	co_await send_msg(types::RECV_FILE, to_string(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		return recv_file_result{ true, result.err_msg };

	co_await send_stream(types::BLOB, in, size);
	co_return recv_file_result{ false };
}

task<session::send_file_result> session::send_file(const std::filesystem::path &path, std::ostream &out)
{
	co_await send_msg(types::SEND_FILE, to_string(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		return send_file_result{ true, result.err_msg };

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

task<session::quit_result> session::quit()
{
	co_return co_await quit("");
}

task<session::quit_result> session::quit(const std::string &msg)
{
	co_await send_msg(types::QUIT, msg);
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}