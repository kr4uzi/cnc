#include "cmd_relay_protocol.h"
#include "serialize_filesystem.h"
#include <type_traits>
#include <sstream>
using namespace cnc;

std::string common::serialize(const cmd_relay_protocol::directory_view &view)
{
	std::ostringstream ss;
	for (const auto &entry : view)
	{
		std::string path = serialize(entry.path);
		std::size_t path_length = path.length();
		auto type = static_cast<std::underlying_type_t<std::filesystem::file_type>>(entry.status.type());
		auto perms = static_cast<std::underlying_type_t<std::filesystem::perms>>(entry.status.permissions());		

		ss << path_length << path;
		ss << type << perms;
	}

	return ss.str();
}

template<>
common::cmd_relay_protocol::directory_view common::deserialize<common::cmd_relay_protocol::directory_view>(const std::string &msg)
{
	std::istringstream ss{ msg };

	cmd_relay_protocol::directory_view result;

	std::size_t path_length = 0;
	ss >> path_length;

	while (path_length)
	{
		std::string path;
		path.resize(path_length);
		ss.read(path.data(), path_length);

		std::underlying_type_t<std::filesystem::file_type> type;
		std::underlying_type_t<std::filesystem::perms> perms;		
		type = static_cast<decltype(type)>(std::filesystem::file_type::none);
		perms = static_cast<decltype(perms)>(std::filesystem::perms::none);
		
		ss >> type >> perms;
		std::filesystem::file_status status(
			static_cast<std::filesystem::file_type>(type),
			static_cast<std::filesystem::perms>(perms)			
		);

		result.push_back({ deserialize<std::filesystem::path>(path), status });

		path_length = 0;
		ss >> path_length;
	}	

	return result;
}