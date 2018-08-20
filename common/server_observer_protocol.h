#pragma once
#include "default_deserialize.h"
#include "server_client_protocol.h"
#include "header.h"
#include "mac_addr.h"
#include <boost/asio/ip/address.hpp>
#include <boost/serialization/access.hpp>
#include <string>
#include <ctime>
#include <vector>
#include <map>
#include <type_traits>

namespace cnc { namespace common { namespace server { namespace observer { 
	struct protocol
	{
		enum class types : std::uint8_t
		{
			FIRST_MEMBER_UNUSED,

			// answer types
			OK,
			ERR, // ERROR

			// request types (* are async requests)
			HELLO,			// server: payload: none, answer: {OK + client_data[]}|{ERROR + string}
			RECV_FILE,		// payload: path, answer: OK|{ERROR + string}
			SEND_FILE,		// payload: path, answer: OK|{ERROR + string}
			BLOB,			// payload: byte[], answer: none
			REGISTERED,		// observer*: payload: client_data, answer: none
			UNREGISTERED,	// observer*: payload: mac, answer: none
			OBSERVE,		// server: payload: mac, answer: {OK + client_log[]}|{ERROR + string}
			LOG,			// observer*: payload: mac + client_log, answer: none			
			UNOBSERVE,		// server: payload: mac, answer: OK|{ERROR + string}
			CONNECT,		// server: payload: connect_data, answer: OK|{ERROR + string}
			QUIT,			// payload: none|string, answer: OK|{ERROR + string}

			LAST_MEMBER_UNUSED
		};

		static const std::map<std::underlying_type<types>::type, std::string> &types_to_string();

		static constexpr unsigned short tcp_port = 5001;
		static constexpr std::uint8_t magic_byte = 0x2;
		using header = cnc::common::header<protocol>;

		struct log
		{
			mac_addr mac;
			std::time_t time;
			std::string type;
			std::string msg;

		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive &ar, const unsigned int version)
			{
				ar & mac;
				ar & time;
				ar & type;
				ar & msg;
			}
		};

		struct connect_data
		{
			mac_addr target_mac;
			boost::asio::ip::address dest_ip;
			unsigned short dest_port;

		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive &ar, const unsigned int version)
			{
				ar & target_mac;
				ar & dest_ip;
				ar & dest_port;
			}
		};

		struct client_data
		{
			client::protocol::hello_data hello_data;
			boost::asio::ip::address ip;

		private:
			friend class boost::serialization::access;
			template<class Archive>
			void serialize(Archive &ar, const unsigned int version)
			{
				ar & hello_data;
				ar & ip;
			}
		};

		using clients = std::vector<client_data>;
		using logs = std::vector<log>;
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
} } } }

namespace cnc { namespace common {
	std::string serialize(const server::observer::protocol::log &log);
	template<>
	server::observer::protocol::log deserialize<server::observer::protocol::log>(const std::string &str);

	std::string serialize(const server::observer::protocol::connect_data &data);
	template<>
	server::observer::protocol::connect_data deserialize<server::observer::protocol::connect_data>(const std::string &str);

	std::string serialize(const server::observer::protocol::client_data &data);
	template<>
	server::observer::protocol::client_data deserialize<server::observer::protocol::client_data>(const std::string &str);

	std::string serialize(const server::observer::protocol::clients &clients);
	template<>
	server::observer::protocol::clients deserialize<server::observer::protocol::clients>(const std::string &str);

	std::string serialize(const server::observer::protocol::logs &logs);
	template<>
	server::observer::protocol::logs deserialize<server::observer::protocol::logs>(const std::string &str);
} }