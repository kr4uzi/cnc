#include "serialize_filesystem.h"

std::string cnc::common::serialize(const std::filesystem::path &path)
{
	return path.u8string();
}

template<>
std::filesystem::path cnc::common::deserialize<std::filesystem::path>(const std::string &msg)
{
	return std::filesystem::u8path(msg);
}