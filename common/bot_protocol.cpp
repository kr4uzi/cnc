#include "bot_protocol.h"
#include "serialize_network.h"
#include <sstream>
using namespace cnc;

std::string common::serialize(const bot_protocol::connect_data &req)
{
	std::ostringstream ss;
	std::string ip_str{ serialize(req.target_ip) };
	ss << ip_str.length() << ip_str;
	ss << req.target_port;
	return ss.str();
}

template<>
common::bot_protocol::connect_data common::deserialize<common::bot_protocol::connect_data>(const std::string &str)
{
	std::istringstream ss{ str };
	std::string::size_type ip_length = 0;
	std::string ip_str;
	ss >> ip_length;
	ip_str.resize(ip_length);
	ss.read(ip_str.data(), ip_length);
	unsigned short port = 0;
	ss >> port;

	return bot_protocol::connect_data{ deserialize<boost::asio::ip::address>(ip_str), port };
}

std::string common::serialize(const bot_protocol::hello_data &data)
{
	return serialize(data.mac);
}

template<>
common::bot_protocol::hello_data common::deserialize<common::bot_protocol::hello_data>(const std::string &str)
{
	return { deserialize<mac_addr>(str) };
}