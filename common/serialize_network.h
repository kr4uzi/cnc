#pragma once
#include "mac_addr.h"
#include "default_deserialize.h"
#include <array>
#include <string>
#include <boost/asio/ip/address.hpp>
#include <boost/serialization/level.hpp>
#include <boost/serialization/array.hpp>

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive &ar, boost::asio::ip::address &addr, unsigned int version)
		{
			std::string str;

			if (Archive::is_saving::value)
				str = addr.to_string();

			ar & boost::serialization::make_nvp("addr", str);

			if (Archive::is_loading::value)
				addr = boost::asio::ip::address::from_string(str);
		}
	}
}

namespace cnc {
	namespace common {
		std::string serialize(const mac_addr &addr);

		template<>
		mac_addr deserialize<mac_addr>(const std::string &str);
	}
}