#pragma once
#include "header.h"
#include "mac_addr.h"
#include "default_deserialize.h"
#include <ostream>
#include <boost/asio/ip/address.hpp>
#include <boost/serialization/access.hpp>

namespace cnc { namespace common { namespace server { namespace client {
	struct protocol
	{
		enum class types : std::uint8_t
		{
			FIRST_MEMBER_UNUSED,

			// answer types
			OK,
			ERR, // ERROR

			// request types (* are async requests)
			HELLO,			// payload: hello_data, answer: OK|{ERROR + string}
			RECV_FILE,		// payload: path, answer: OK|{ERROR + string}
			SEND_FILE,		// payload: path, answer: OK|{ERROR + string}
			BLOB,			// payload: byte[], answer: none
			CONNECT,		// payload: connect_data, answer: OK|{ERROR + string}
			QUIT,			// payload: none|string, answer: OK|{ERROR + string}

			LAST_MEMBER_UNUSED
		};

		static constexpr unsigned short tcp_port = 5000;
		static constexpr std::uint8_t magic_byte = 0x1;
		using header = cnc::common::header<protocol>;

		struct connect_data
		{
			boost::asio::ip::address target_ip;
			unsigned short target_port;

		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive &ar, const unsigned int version)
			{
				ar & target_ip;
				ar & target_port;
			}
		};

		struct hello_data
		{
			mac_addr mac;

		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive &ar, const unsigned int version)
			{
				ar & mac;
			}
		};
	};

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const protocol::types &type)
	{
		using types = protocol::types;
		switch (type)
		{
		case types::OK:			return os << "OK";
		case types::ERR:		return os << "ERR";

		case types::HELLO:		return os << "HELLO";
		case types::RECV_FILE:	return os << "RECV_FILE";
		case types::SEND_FILE:	return os << "SEND_FILE";
		case types::BLOB:		return os << "BLOB";
		case types::CONNECT:	return os << "CONNECT";
		case types::QUIT:		return os << "QUIT";
		}

		throw std::runtime_error("unknown client::protocol type");
	}
} } } }

namespace cnc { namespace common {
	std::string serialize(const server::client::protocol::connect_data &req);

	template<>
	server::client::protocol::connect_data deserialize<server::client::protocol::connect_data>(const std::string &str);

	std::string serialize(const server::client::protocol::hello_data &entry);
	template<>
	server::client::protocol::hello_data deserialize<server::client::protocol::hello_data>(const std::string &str);
} }