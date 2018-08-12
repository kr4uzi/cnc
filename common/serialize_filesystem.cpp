#include "serialize_filesystem.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>

std::string cnc::common::serialize(const std::filesystem::path &path)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << path.native();
	return ss.str();
}

template<>
std::filesystem::path cnc::common::deserialize<std::filesystem::path>(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	std::filesystem::path::string_type str;
	ia >> str;
	return std::filesystem::path(str);
}