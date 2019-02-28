#pragma once
#include <cstddef>
#include <array>
#include <string>
#include <vector>
#include <ostream>

namespace cnc { namespace common {
	typedef std::array<unsigned char, 6> mac_addr;

	std::string to_string(const mac_addr &addr);

	std::vector<mac_addr> get_mac_addresses();

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const mac_addr &addr)
	{
		return os << to_string(addr).c_str();
	}
} }
