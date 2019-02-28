#include "cmd_protocol.h"
#include "serialize_network.h"
#include <sstream>
using namespace cnc;

std::string common::serialize(const cmd_protocol::client_data &entry)
{	
	std::string hello_data{ common::serialize(entry.hello_data) };

	std::ostringstream ss;
	ss << hello_data.length() << hello_data;
	ss << common::serialize(entry.ip);
	return ss.str();
}

template<>
common::cmd_protocol::client_data common::deserialize<common::cmd_protocol::client_data>(const std::string &msg)
{
	std::istringstream ss{ msg };

	std::string::size_type length = 0;
	ss >> length;
	std::string hello_data_str;
	hello_data_str.resize(length);
	ss.read(hello_data_str.data(), length);

	auto hello_data = deserialize<bot_protocol::hello_data>(hello_data_str);
	auto ip = deserialize<boost::asio::ip::address>(ss.str());
	return { hello_data, ip };
}

std::string common::serialize(const cmd_protocol::log &data)
{
	std::ostringstream ss;
	std::string mac{ serialize(data.mac) };
	ss << mac.length() << mac;
	ss << time;
	ss << data.type.length() << data.type;
	ss << data.msg;
	return ss.str();
}

template<>
common::cmd_protocol::log common::deserialize<common::cmd_protocol::log>(const std::string &msg)
{
	std::istringstream ss{ msg };

	std::string::size_type length = 0;
	ss >> length;
	std::string mac_str(length, '\0');
	ss.read(mac_str.data(), length);
	auto mac = deserialize<common::mac_addr>(mac_str);

	decltype(cmd_protocol::log::time) time = 0;
	ss >> time;

	length = 0;
	ss >> length;
	std::string type(length, '\0');
	ss.read(type.data(), length);

	return { mac, time, type, ss.str() };
}

std::string common::serialize(const cmd_protocol::clients &clients)
{
	std::ostringstream ss;
	for (const auto &client : clients)
	{
		std::string client_str{ serialize(clients) };
		ss << client_str.length() << client_str;
	}

	return ss.str();
}

template<>
common::cmd_protocol::clients common::deserialize<common::cmd_protocol::clients>(const std::string &str)
{
	std::istringstream ss{ str };
	cmd_protocol::clients clients;
	std::string::size_type length = 0;
	ss >> length;
	while (length)
	{
		std::string client_str(length, '\0');
		ss.read(client_str.data(), length);

		length = 0;
		ss >> length;
	}

	return clients;
}

std::string common::serialize(const cmd_protocol::logs &logs)
{
	std::ostringstream ss;
	for (const auto &log : logs)
	{
		std::string log_str{ serialize(log) };
		ss << log_str.length() << log_str;
	}

	return ss.str();
}

template<>
common::cmd_protocol::logs common::deserialize<common::cmd_protocol::logs>(const std::string &str)
{
	std::istringstream ss{ str };
	cmd_protocol::logs logs;
	std::string::size_type length = 0;
	ss >> length;
	while (length)
	{
		std::string log_str(length, '\0');
		ss.read(log_str.data(), length);

		length = 0;
		ss >> length;
	}

	return logs;
}