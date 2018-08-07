#include "serialize_network.h"
#include <boost/serialization/array.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>

std::string cnc::common::to_string(const cnc::common::mac_addr &addr)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << addr;
	return ss.str();
}

cnc::common::mac_addr cnc::common::mac_addr_from_string(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	mac_addr addr;
	ia >> addr;
	return addr;
}