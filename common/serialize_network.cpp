#include "serialize_network.h"
using namespace cnc;

std::string common::serialize(const boost::asio::ip::address &addr)
{
	return addr.to_string();
}

std::string common::serialize(const common::mac_addr &addr)
{
	std::vector<unsigned char> bytes = { addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] };
	return std::string(bytes.begin(), bytes.end());
}

template<>
boost::asio::ip::address common::deserialize<boost::asio::ip::address>(const std::string &str)
{
	return boost::asio::ip::address::from_string(str);
}

template<>
common::mac_addr common::deserialize<common::mac_addr>(const std::string &str)
{
	if (str.length() != 6)
		throw std::runtime_error("mac addr string must be of length 6");

	mac_addr addr;
	for (std::size_t i = 0; i < 6; i++)
		addr[i] = str[i];

	return addr;
}