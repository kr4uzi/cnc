#pragma once
#include "bot_protocol.h"
#include "default_deserialize.h"
#include "serialize_network.h"
#include "serialize_filesystem.h"
#include "header.h"
#include "mac_addr.h"
#include <boost/asio/ip/address.hpp>
#include <string>
#include <ctime>
#include <vector>
#include <map>
#include <type_traits>

namespace cnc { namespace common {
	struct cmd_protocol
	{
		enum class types
		{
			// answer types
			OK,
			ERR, // ERROR

			// request types (* are out of order messages)
			HELLO,			// server: payload: none, answer: {OK + client_data[]}|{ERROR + string}
			RECV_FILE,		// payload: path, answer: OK|{ERROR + string}
			SEND_FILE,		// payload: path, answer: OK|{ERROR + string}
			BLOB,			// payload: byte[], answer: none
			REGISTERED,		// observer*: payload: client_data, answer: none
			UNREGISTERED,	// observer*: payload: mac, answer: none
			OBSERVE,		// server: payload: mac, answer: OK|{ERROR + string}
			LOG,			// observer*: payload: mac + client_log, answer: none
			UNOBSERVE,		// server: payload: mac, answer: OK|{ERROR + string}
			CONNECT,		// server: payload: connect_data, answer: OK|{ERROR + string}
			QUIT			// payload: none|string, answer: OK|{ERROR + string}
		};

		static constexpr std::uint8_t magic_byte = 0xAC;

		struct log
		{
			mac_addr mac;
			std::time_t time;
			std::string type;
			std::string msg;
		};

		struct client_data
		{
			bot_protocol::hello_data hello_data;
			boost::asio::ip::address ip;
		};

		typedef std::vector<client_data> clients;
		typedef std::vector<log> logs;
	};

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const cmd_protocol::types &type)
	{
		using types = cmd_protocol::types;
		switch (type)
		{
		case types::OK:			return os << "OK";
		case types::ERR:		return os << "ERR";

		case types::HELLO:		return os << "HELLO";
		case types::REGISTERED:	return os << "REGISTERED";
		case types::UNREGISTERED: return os << "UNREGISTERED";
		case types::OBSERVE:	return os << "OBSERVE";
		case types::LOG:		return os << "LOG";
		case types::UNOBSERVE:	return os << "UNOBSERVE";
		case types::CONNECT:	return os << "CONNECT";
		case types::QUIT:		return os << "QUIT";
		}

		throw std::runtime_error("unknown client::protocol type");
	}

	std::string serialize(const cmd_protocol::log &log);
	template<>
	cmd_protocol::log deserialize<cmd_protocol::log>(const std::string &str);

	std::string serialize(const cmd_protocol::client_data &data);
	template<>
	cmd_protocol::client_data deserialize<cmd_protocol::client_data>(const std::string &str);

	std::string serialize(const cmd_protocol::clients &clients);
	template<>
	cmd_protocol::clients deserialize<cmd_protocol::clients>(const std::string &str);

	std::string serialize(const cmd_protocol::logs &logs);
	template<>
	cmd_protocol::logs deserialize<cmd_protocol::logs>(const std::string &str);
} }