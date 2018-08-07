// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

// Headers for CppUnitTest
#include "CppUnitTest.h"

// TODO: reference additional headers your program requires here
#include <string>
#include <vector>
#include <filesystem>
#include <boost/serialization/access.hpp>
#include <type_traits>
#include <common/client_protocol.h>
#include <common/client_session.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <common/header.h>
#include <common/basic_session.h>
#include <common/serialize_network.h>
#include <common/serialize_filesystem.h>
#include <common/server_observer_protocol.h>
#include <common/server_observer_session.h>
#include <common/server_client_protocol.h>
#include <common/server_client_session.h>
#include <sstream>

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
	inline std::wstring ToString(const cnc::mac_addr &addr)
	{
		std::wstringstream ss;
		for (auto & byte : addr)
			ss << std::setw(2) << std::setfill(L'0') << (int)byte << L":";

		return ss.str();
	}

	inline std::wstring ToString(const boost::asio::ip::address &addr)
	{
		auto str = addr.to_string();
		return std::wstring(str.begin(), str.end());
	}

	inline std::wstring ToString(const std::filesystem::path &path)
	{
		return path.wstring();
	}

	inline std::wstring ToString(const cnc::server::client::protocol::hello_data &data)
	{
		return ToString(data.mac);
	}

	inline std::wstring ToString(const cnc::server::client::protocol::connect_request &req)
	{
		std::wstringstream ss;
		ss << ToString(req.target_ip) << ':' << req.target_port;
		return ss.str();
	}

	inline std::wstring ToString(const cnc::server::observer::protocol::request_connect &req)
	{
		std::wstringstream ss;
		ss << ToString(req.target_mac) << ":" << ToString(req.dest_ip) << ":" << req.dest_port;
		return ss.str();
	}

	inline std::wstring ToString(const cnc::server::observer::protocol::client_data &data)
	{
		std::wstringstream ss;
		ss << ToString(data.hello_data) << ":" << ToString(data.ip);
		return ss.str();
	}

	inline std::wstring ToString(const cnc::server::observer::protocol::clients &clients)
	{
		using clients_t = cnc::server::observer::protocol::clients;
		std::wstringstream ss;
		clients_t::size_type i = 0;
		for (; i < clients.size() && i <= 5; ++i)
		{
			ss << ToString(clients[i]);

			if (i < 5 && i < clients.size())
				ss << ", ";
		}

		if (i == 5 && i < clients.size())
			ss << ", ...";

		return ss.str();
	}

	inline std::wstring ToString(const cnc::server::observer::protocol::log &log)
	{
		std::wstringstream ss;
		ss << ToString(log.mac) << ":"
			<< log.time << ":"
			<< std::wstring(log.type.begin(), log.type.end()) << ":"
			<< std::wstring(log.msg.begin(), log.msg.end());
		return ss.str();
	}

	inline std::wstring ToString(const cnc::server::observer::protocol::logs &logs)
	{
		using logs_t = cnc::server::observer::protocol::logs;
		std::wstringstream ss;
		logs_t::size_type i = 0;
		for (; i < logs.size() && i <= 5; ++i)
		{
			ss << ToString(logs[i]);

			if (i < 5 && i < logs.size())
				ss << ", ";
		}

		if (i == 5 && i < logs.size())
			ss << ", ...";

		return ss.str();
	}
} } }