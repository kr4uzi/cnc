#include "server_client_protocol.h"
#include "serialize_network.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc::common::server::client;

std::string protocol::to_string(const connect_data &req)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << req;
	return ss.str();
}

protocol::connect_data protocol::connect_data_from_string(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	connect_data data;
	ia >> data;
	return data;
}

std::string protocol::to_string(const hello_data &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

protocol::hello_data protocol::hello_data_from_string(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	hello_data data;
	ia >> data;
	return data;
}