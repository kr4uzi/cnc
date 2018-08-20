#pragma once
#include <array>
#include <string>
#include <vector>
#include <ostream>

namespace cnc { namespace common {
	struct mac_addr : std::array<unsigned char, 6>
	{
		using base_type = std::array<unsigned char, 6>;
		static constexpr std::size_t bytes = std::tuple_size<base_type>::value;
	};
	//using mac_addr = std::array<unsigned char, 6>;

	std::string to_string(const mac_addr &addr);

	std::vector<mac_addr> get_mac_addresses();

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const mac_addr &addr)
	{
		return os << to_string(addr).c_str();
	}
} }
