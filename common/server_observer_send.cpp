#include "server_observer_send.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"

using namespace cnc::common;
using namespace cnc::common::server::observer;
using types = protocol::types;
using boost::asio::ip::tcp;

task<send::err_or_empty_ok_result> send::recv_err_or_empty_ok(tcp::socket &socket, const protocol::header &response)
{
	switch (response.get_type())
	{
	case types::OK:
		if (response.get_payload_size() != 0)
			throw std::runtime_error("unexpected_message_error");
			//throw unexpected_message_error(*this, response, "no payload expected");

		co_return err_or_empty_ok_result{ false };

	case types::ERR:
		co_return err_or_empty_ok_result{ true, co_await session::recv_msg(socket, response.get_payload_size()) };
	}

	throw std::runtime_error("unexpected_message_error");
	//throw unexpected_message_error(*this, response);
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
	//throw unexpected_message_error(*this, response);
}

task<send::observe_result> send::observe(tcp::socket &socket, const cnc::common::mac_addr &mac)
{
	co_await session::send_msg(socket, types::OBSERVE, to_string(mac));
	auto response = co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
	if (response.err)
		co_return observe_result{ true, response.err_msg };

	co_return observe_result{ false, "", deserialize<protocol::logs>(response.msg) };
}

task<send::hello_result> send::hello(tcp::socket &socket)
{
	co_await session::send_msg(socket, types::OBSERVE);
	auto response = co_await recv_err_or_ok(socket, co_await session::recv_header(socket));
	if (response.err)
		co_return hello_result{ true, response.err_msg };

	co_return hello_result{ false, "", deserialize<protocol::clients>(response.msg) };
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
		//throw unexpected_message_error(*this, response);

	co_await session::recv_stream(socket, out, response.get_payload_size());
	co_return send_file_result{ false };
}

task<send::unobserve_result> send::quit(tcp::socket &socket)
{
	co_await session::send_msg(socket, types::QUIT);
	co_return co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
}

task<send::unobserve_result> send::quit(tcp::socket &socket, const std::string &message)
{
	co_await session::send_msg(socket, types::QUIT, message);
	co_return co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
}

task<send::unobserve_result> send::unobserve(tcp::socket &socket, const cnc::common::mac_addr &mac)
{
	co_await session::send_msg(socket, types::UNOBSERVE, to_string(mac));
	co_return co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
}

task<send::connect_result> send::connect(tcp::socket &socket, const protocol::connect_data &data)
{
	co_await session::send_msg(socket, types::CONNECT, serialize(data));
	co_return co_await recv_err_or_empty_ok(socket, co_await session::recv_header(socket));
}