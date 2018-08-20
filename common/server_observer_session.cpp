#include "server_observer_session.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc::common;
using namespace cnc::common::server::observer::send;
using boost::asio::ip::tcp;
using types = session::protocol::types;

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
			throw std::runtime_error("unexpected_message_error");
			//throw unexpected_message_error(*this, response, "no payload expected");

		co_return err_or_empty_ok_result{ false };

	case types::ERR:
		co_return err_or_empty_ok_result{ true, co_await recv_msg(response.get_payload_size()) };
	}

	throw std::runtime_error("unexpected_message_error");
	//throw unexpected_message_error(*this, response);
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

	throw std::runtime_error("unexpected_message_error");
	//throw unexpected_message_error(*this, response);
}

task<session::observe_result> session::observe(const cnc::common::mac_addr &mac)
{
	co_await send_msg(types::OBSERVE, to_string(mac));
	auto response = co_await recv_err_or_ok(co_await recv_header());
	if (response.err)
		co_return observe_result{ true, response.err_msg };

	co_return observe_result{ false, "", deserialize<protocol::logs>(response.msg) };
}

task<session::hello_result> session::hello()
{
	co_await send_msg(types::OBSERVE);
	auto response = co_await recv_err_or_ok(co_await recv_header());
	if (response.err)
		co_return hello_result{ true, response.err_msg };

	co_return hello_result{ false, "", deserialize<protocol::clients>(response.msg) };
}

task<session::recv_file_result> session::recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size)
{
	co_await send_msg(types::RECV_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		co_return recv_file_result{ true, result.err_msg };

	co_await send_stream(types::BLOB, in, size);
	co_return recv_file_result{ false };
}

task<session::send_file_result> session::send_file(const std::filesystem::path &path, std::ostream &out)
{
	co_await send_msg(types::SEND_FILE, serialize(path));
	auto result = co_await recv_err_or_empty_ok(co_await recv_header());
	if (result.err)
		co_return send_file_result{ true, result.err_msg };

	auto response = co_await recv_header();
	if (response.get_type() != types::BLOB)
		throw std::runtime_error("unexpected_message_error");
		//throw unexpected_message_error(*this, response);

	co_await recv_stream(out, response.get_payload_size());
	co_return send_file_result{ false };
}

task<session::unobserve_result> session::quit()
{
	co_await send_msg(types::QUIT);
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

task<session::unobserve_result> session::quit(const std::string &message)
{
	co_await send_msg(types::QUIT, message);
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

task<session::unobserve_result> session::unobserve(const cnc::common::mac_addr &mac)
{
	co_await send_msg(types::UNOBSERVE, to_string(mac));
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}

task<session::connect_result> session::connect(const protocol::connect_data &data)
{
	co_await send_msg(types::CONNECT, serialize(data));
	co_return co_await recv_err_or_empty_ok(co_await recv_header());
}