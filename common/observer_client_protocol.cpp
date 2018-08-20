#include "observer_client_protocol.h"
#include "serialize_tuple.h"
#include "serialize_filesystem.h"
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc;
using namespace cnc::common::observer::client;

std::string common::serialize(const protocol::directory_view &view)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << view;
	return ss.str();
}

template<>
protocol::directory_view common::deserialize<protocol::directory_view>(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	protocol::directory_view view;
	ia >> view;
	return view;
}