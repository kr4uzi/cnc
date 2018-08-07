#include "server_observer_session.h"
#include "serialize_network.h"
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc::common::server::observer;
using boost::asio::ip::tcp;
using types = protocol::types;

session::session(tcp::socket socket)
	: basic_session<protocol>(std::move(socket))
{

}

session::err_or_empty_ok_result session::recv_err_or_empty_ok(const protocol::header &response, boost::asio::yield_context ctx)
{
	switch (response.get_type())
	{
	case types::OK:
		if (response.get_payload_size() != 0)
			throw unexpected_message_error(*this, response, "no payload expected");

		return { false };

	case types::ERR:
		return { true, recv_msg(response.get_payload_size(), ctx) };
	}

	throw unexpected_message_error(*this, response);
}

session::err_or_ok_result session::recv_err_or_ok(const protocol::header &response, boost::asio::yield_context ctx)
{
	switch (response.get_type())
	{
	case types::OK:
		return { false, "", recv_msg(response.get_payload_size(), ctx) };
	case types::ERR:
		return { true, recv_msg(response.get_payload_size(), ctx) };
	}

	throw unexpected_message_error(*this, response);
}

session::observe_result session::observe(const cnc::common::mac_addr &mac, boost::asio::yield_context yield)
{
	send_msg(types::OBSERVE, to_string(mac), yield);
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

	
	return { false, "", protocol::logs_from_string(recv_msg(response.get_payload_size(), yield)) };
}

session::hello_result session::hello(boost::asio::yield_context yield)
{
	send_msg(types::OBSERVE, yield);
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

	return { false, "", protocol::clients_from_string(recv_msg(response.get_payload_size(), yield)) };
}

session::unobserve_result session::quit(boost::asio::yield_context yield)
{
	send_msg(types::QUIT, yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}

session::unobserve_result session::quit(const std::string &message, boost::asio::yield_context yield)
{
	send_msg(types::QUIT, message, yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}

session::unobserve_result session::unobserve(const cnc::common::mac_addr &mac, boost::asio::yield_context yield)
{
	send_msg(types::UNOBSERVE, to_string(mac), yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}
session::connect_result session::connect(const protocol::connect_data &data, boost::asio::yield_context yield)
{
	send_msg(types::CONNECT, protocol::to_string(data), yield);
	return recv_err_or_empty_ok(recv_header(yield), yield);
}