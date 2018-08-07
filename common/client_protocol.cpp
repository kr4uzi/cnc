#include "client_protocol.h"
#include "serialize_tuple.h"
#include "serialize_filesystem.h"
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc::common::client;

std::string protocol::to_string(const directory_view &view)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << view;
	return ss.str();
}

protocol::directory_view protocol::directory_view_from_string(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	directory_view view;
	ia >> view;
	return view;
}