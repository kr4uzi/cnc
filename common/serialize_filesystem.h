#pragma once
#include "default_deserialize.h"
#include <filesystem>

namespace cnc { namespace common {
	std::string serialize(const std::filesystem::path &path);

	template<>
	std::filesystem::path deserialize<std::filesystem::path>(const std::string &str);
} }