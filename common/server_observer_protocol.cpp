#include "server_observer_protocol.h"
#include "serialize_tuple.h"
#include "serialize_network.h"
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc::common::server::observer;

std::string protocol::to_string(const client_data &entry)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << entry;
	return ss.str();
}

protocol::client_data protocol::client_data_from_string(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	client_data entry;
	ia >> entry;
	return entry;
}

std::string protocol::to_string(const log &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

protocol::log protocol::log_from_string(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	log data;
	ia >> data;
	return data;
}

std::string protocol::to_string(const connect_data &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

protocol::connect_data protocol::connect_data_from_string(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	connect_data data;
	ia >> data;
	return data;
}

std::string protocol::to_string(const clients &clients)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << clients;
	return ss.str();
}

protocol::clients protocol::clients_from_string(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	clients data;
	ia >> data;
	return data;
}

std::string protocol::to_string(const logs &logs)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << logs;
	return ss.str();
}

protocol::logs protocol::logs_from_string(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	logs data;
	ia >> data;
	return data;
}