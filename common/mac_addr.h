#pragma once
#include <array>
#include <string>

namespace cnc {
	namespace common {
		using mac_addr = std::array<unsigned char, 6>;

		std::string to_string(const mac_addr &addr);
	}
}
