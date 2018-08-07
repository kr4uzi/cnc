#include "server_client_session.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using namespace cnc::common::server::client;
using types = protocol::types;

session::session(tcp::socket socket)
	: basic_session<protocol>(std::move(socket))
{

}

session::err_or_empty_ok_result session::recv_err_or_empty_ok(const protocol::header &response, yield_context yield)
{
	switch (response.get_type())
	{
	case types::OK:
		if (response.get_payload_size() != 0)
			throw unexpected_message_error(*this, response, "no payload expected");

		return { false };

	case types::ERR:
		return { true, recv_msg(response.get_payload_size(), yield) };
	}

	throw unexpected_message_error(*this, response);
}

session::err_or_ok_result session::recv_err_or_ok(const protocol::header &response, yield_context yield)
{
	switch (response.get_type())
	{
	case types::OK:
		return { false, "", recv_msg(response.get_payload_size(), yield) };
	case types::ERR:
		return { true, recv_msg(response.get_payload_size(), yield) };
	}

	throw unexpected_message_error(*this, response);
}

session::hello_result session::hello(const protocol::hello_data &data, boost::asio::yield_context yield)
{
	send_msg(types::HELLO, protocol::to_string(data), yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}

session::connect_result session::connect(const protocol::connect_data &data, boost::asio::yield_context yield)
{
	send_msg(types::CONNECT, protocol::to_string(data), yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}

session::recv_file_result session::recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size, yield_context yield)
{
	send_msg(types::RECV_FILE, to_string(path), yield);
	auto result = recv_err_or_empty_ok(recv_header(yield), yield);
	if (result.err)
		return { true, result.err_msg };

	send_stream(types::BLOB, in, size, yield);
	return { false };
}

session::send_file_result session::send_file(const std::filesystem::path &path, std::ostream &out, yield_context yield)
{
	send_msg(types::SEND_FILE, to_string(path), yield);
	auto result = recv_err_or_empty_ok(recv_header(yield), yield);
	if (result.err)
		return { true, result.err_msg };

	auto response = recv_header(yield);
	switch (response.get_type())
	{
	case types::BLOB:
		break;
	default:
		throw unexpected_message_error(*this, response);
	}

	recv_stream(out, response.get_payload_size(), yield);
	return { false };
}

session::quit_result session::quit(boost::asio::yield_context yield)
{
	return quit("", yield);
}

session::quit_result session::quit(const std::string &msg, boost::asio::yield_context yield)
{
	send_msg(types::QUIT, msg, yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}