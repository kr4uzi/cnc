#pragma once
#include "mac_addr.h"
#include "default_deserialize.h"
#include <boost/asio/ip/address.hpp>

namespace cnc { namespace common {
	std::string serialize(const boost::asio::ip::address &addr);
	std::string serialize(const mac_addr &addr);

	template<>
	boost::asio::ip::address deserialize<boost::asio::ip::address>(const std::string &str);

	template<>
	mac_addr deserialize<mac_addr>(const std::string &str);
} }