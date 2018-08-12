#include "server_client_protocol.h"
#include "serialize_network.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc;
using protocol = common::server::client::protocol;

std::string common::serialize(const protocol::connect_data &req)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << req;
	return ss.str();
}

template<>
protocol::connect_data common::deserialize<protocol::connect_data>(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	protocol::connect_data data;
	ia >> data;
	return data;
}

std::string common::serialize(const protocol::hello_data &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

template<>
protocol::hello_data common::deserialize<protocol::hello_data>(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	protocol::hello_data data;
	ia >> data;
	return data;
}