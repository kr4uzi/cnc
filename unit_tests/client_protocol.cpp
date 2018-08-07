#define BOOST_TEST_MODULE client_protocol
#include <boost/test/unit_test.hpp>
#include "../common/client_protocol.h"
#include "client_print.h"
using cnc::client::protocol;
using types = protocol::types;

namespace test {
	namespace client {
		namespace protocol {
			BOOST_AUTO_TEST_CASE(directory_view)
			{
				::protocol::directory_view view;
				namespace fs = std::filesystem;
				for (unsigned i = 0; i < 128; i++)
					view.push_back({ fs::path("folder" + std::to_string(i)), fs::file_status(fs::file_type::directory) });

				for (unsigned i = 0; i < 128; i++)
					view.push_back({ fs::path("file" + std::to_string(i)), fs::file_status(fs::file_type::regular) });

				BOOST_TEST(std::get<1>(view[0]) == std::get<1>(view[0]));
				auto msg = ::protocol::to_string(view);
				decltype(view) deserialized_view = ::protocol::directory_view_from_string(msg);
				BOOST_TEST(view == deserialized_view);
			}

			BOOST_AUTO_TEST_CASE(header_encode_decode)
			{
				using types_t = std::underlying_type<types>::type;

				for (types_t type = 0; type < static_cast<types_t>(types::LAST_UNUSED_MEMBER); type++)
				{
					for (std::uint64_t size_log = 0; size_log < 64; size_log++)
					{
						std::uint64_t size = 1ull << size_log;

						::protocol::header encoded(static_cast<types>(type), size);
						BOOST_TEST_REQUIRE(encoded.is_valid());
						BOOST_TEST(encoded.get_payload_size() == size);
						BOOST_TEST(static_cast<types_t>(encoded.get_type()) == type);

						auto bytes = encoded.to_bytearray();
						::protocol::header decoded(bytes);
						BOOST_TEST_REQUIRE(decoded.is_valid());
						BOOST_TEST(decoded.get_payload_size() == size);
						BOOST_TEST(static_cast<types_t>(decoded.get_type()) == type);
					}
				}
			}
		}
	}
}