#include "server_observer_protocol.h"
#include "serialize_tuple.h"
#include "serialize_network.h"
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <sstream>
using namespace cnc;
using protocol = common::server::observer::protocol;

std::string common::serialize(const protocol::client_data &entry)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << entry;
	return ss.str();
}

template<>
protocol::client_data common::deserialize<protocol::client_data>(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	protocol::client_data entry;
	ia >> entry;
	return entry;
}

std::string common::serialize(const protocol::log &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

template<>
protocol::log common::deserialize<protocol::log>(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	protocol::log data;
	ia >> data;
	return data;
}

std::string common::serialize(const protocol::connect_data &data)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << data;
	return ss.str();
}

template<>
protocol::connect_data common::deserialize<protocol::connect_data>(const std::string &msg)
{
	std::istringstream ss{ msg };
	boost::archive::text_iarchive ia{ ss };
	protocol::connect_data data;
	ia >> data;
	return data;
}

std::string common::serialize(const protocol::clients &clients)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << clients;
	return ss.str();
}

template<>
protocol::clients common::deserialize<protocol::clients>(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	protocol::clients data;
	ia >> data;
	return data;
}

std::string common::serialize(const protocol::logs &logs)
{
	std::ostringstream ss;
	boost::archive::text_oarchive oa{ ss };
	oa << logs;
	return ss.str();
}

template<>
protocol::logs common::deserialize<protocol::logs>(const std::string &str)
{
	std::istringstream ss{ str };
	boost::archive::text_iarchive ia{ ss };
	protocol::logs data;
	ia >> data;
	return data;
}

const std::map<std::underlying_type<protocol::types>::type, std::string> &protocol::types_to_string()
{
	static bool initialized;
	static std::map<std::underlying_type<protocol::types>::type, std::string> values;

	if (initialized)
		return values;

	auto first = static_cast<std::underlying_type<types>::type>(types::FIRST_MEMBER_UNUSED);
	auto last = static_cast<std::underlying_type<types>::type>(types::LAST_MEMBER_UNUSED);

	for (auto i = first + 1; i < last; i++)
	{
		std::ostringstream ss;
		ss << static_cast<types>(i);
		values.emplace(i, ss.str());
	}

	return values;
}