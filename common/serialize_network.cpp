#include "serialize_network.h"
#include <boost/serialization/array.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc;

std::string common::serialize(const common::mac_addr &addr)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << addr;
	return ss.str();
}

template<>
common::mac_addr common::deserialize<common::mac_addr>(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	mac_addr addr;
	ia >> addr;
	return addr;
}