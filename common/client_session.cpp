#include "client_session.h"
#include "serialize_filesystem.h"

using namespace cnc::common;
using namespace cnc::common::client;
using boost::asio::yield_context;
using types = protocol::types;

session::session(boost::asio::ip::tcp::socket socket)
	: basic_session<protocol>(std::move(socket))
{

}

session::hello_result session::hello(boost::asio::yield_context yield)
{
	return hello("", yield);
}

session::hello_result session::hello(const std::string &msg, boost::asio::yield_context yield)
{
	send_msg(types::HELLO, msg, yield);
	return recv_err_or_ok(recv_header(yield), yield);
}

session::create_directory_result session::create_directory(const std::filesystem::path &path, boost::asio::yield_context yield)
{
	send_msg(types::CREATE_DIRECTORY, path.string(), yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}

session::list_directory_result session::list_directory(const std::filesystem::path &path, boost::asio::yield_context yield)
{
	send_msg(types::LIST_DIRECTORY, path.string(), yield);
	auto response = recv_header(yield);
	switch (response.get_type())
	{
	case types::OK:
		break;
	case types::ERR:
		return { true, recv_msg(response.get_payload_size(), yield) };
	default:
		throw unexpected_message_error(*this, response);
	}

	return { false, "", protocol::directory_view_from_string(recv_msg(response.get_payload_size(), yield)) };
}

session::recv_file_result session::recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size, boost::asio::yield_context yield)
{
	send_msg(types::RECV_FILE, to_string(path), yield);
	auto result = recv_err_or_empty_ok(recv_header(yield), yield);
	if (result.err)
		return { true, result.err_msg };

	send_stream(types::BLOB, in, size, yield);
	return { false };
}

session::send_file_result session::send_file(const std::filesystem::path &path, std::ostream &out, boost::asio::yield_context yield)
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

session::execute_result session::execute(const std::string &cmd, boost::asio::yield_context yield)
{
	send_msg(types::EXECUTE, cmd, yield);
	auto result = recv_err_or_ok(recv_header(yield), yield);
	return { result.err, result.err_msg, result.msg };
}

session::quit_result session::quit(boost::asio::yield_context yield)
{
	return quit("", yield);
}

session::quit_result session::quit(const std::string &msg, boost::asio::yield_context yield)
{
	send_msg(types::QUIT, msg, yield);
	return recv_err_or_ok(recv_header(yield), yield);
}

session::err_or_empty_ok_result session::recv_err_or_empty_ok(const protocol::header &response, boost::asio::yield_context yield)
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

session::err_or_ok_result session::recv_err_or_ok(const protocol::header &response, boost::asio::yield_context yield)
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