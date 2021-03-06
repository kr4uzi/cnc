#include "converters.h"
#include <pybind11/pybind11.h>
#include <regex>
using namespace pybind11;
using namespace pybind11::detail;

bool type_caster<cnc::common::mac_addr>::load(handle src, bool)
{
	// this allows a mixture of : and - but this is actually allowed by the IEEE standard
	static std::regex exp{ R"REGEX(^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$)REGEX" };

	auto str = src.cast<std::string>();
	if (!std::regex_match(str, exp))
		return false;

	cnc::common::mac_addr addr;
	for (int i = 0; i < 6; i++)
	{
		auto pos = 3 * i;
		addr[i] = ((str[pos] - '0') << 4) | (str[pos + 1] - '0');
	}

	value = addr;
	return true;
}

handle type_caster<cnc::common::mac_addr>::cast(const cnc::common::mac_addr &src, return_value_policy, handle)
{
	return pybind11::str(to_string(src));
}

bool type_caster<cnc::common::server::client::protocol::hello_data>::load(handle src, bool)
{
	try
	{
		auto tuple = src.cast<pybind11::tuple>();
		value.mac = tuple[0].cast<cnc::common::mac_addr>();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

handle type_caster<cnc::common::server::client::protocol::hello_data>::cast(const cnc::common::server::client::protocol::hello_data &src, return_value_policy, handle)
{
	auto dict = pybind11::dict("mac"_a = src.mac);
	dict.inc_ref();
	return dict;

	auto tuple = pybind11::make_tuple(src.mac);
	tuple.inc_ref();
	return tuple;
}