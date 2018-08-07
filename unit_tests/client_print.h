#include <type_traits>
#include <filesystem>
#include <ostream>
#include "../common/client_protocol.h"

namespace std {
	namespace filesystem {
		inline bool operator==(const file_status &lhs, const file_status &rhs)
		{
			return lhs.type() == rhs.type() &&
				lhs.permissions() == rhs.permissions();
		}

		template<class CharT, class Traits>
		basic_ostream<CharT, Traits>& operator<<(basic_ostream<CharT, Traits>& os, const file_status &fs)
		{
			return os << "type: "
				<< static_cast<underlying_type<file_type>::type>(fs.type())
				<< ", perms: "
				<< static_cast<underlying_type<perms>::type>(fs.permissions());
		}
	}
}