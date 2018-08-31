#pragma once
#include <common/mac_addr.h>
#include <common/server_client_protocol.h>
#include <pybind11/cast.h>

namespace pybind11 { namespace detail {
	template<> struct type_caster<cnc::common::mac_addr>
	{
		PYBIND11_TYPE_CASTER(cnc::common::mac_addr, "cnc.common.mac_addr");

		bool load(handle src, bool);
		static handle cast(const cnc::common::mac_addr &src, return_value_policy policy, handle parent);
	};

	template<> struct type_caster<cnc::common::server::client::protocol::hello_data>
	{
		PYBIND11_TYPE_CASTER(cnc::common::server::client::protocol::hello_data, "cnc.common.server.client.protocol.hello_data");

		bool load(handle src, bool);

		static handle cast(const cnc::common::server::client::protocol::hello_data &src, return_value_policy policy, handle parent);
	};
} }