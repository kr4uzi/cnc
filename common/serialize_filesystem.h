#pragma once
#include <string>
#include <filesystem>
#include <type_traits>
#include <boost/serialization/level.hpp>

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive &ar, std::filesystem::file_status &status, unsigned int version)
		{
			std::underlying_type<std::filesystem::file_type>::type type;
			std::underlying_type<std::filesystem::perms>::type perms;

			if (Archive::is_saving::value)
			{
				type = static_cast<decltype(type)>(status.type());
				perms = static_cast<decltype(perms)>(status.permissions());
			}

			ar & boost::serialization::make_nvp("type", type);
			ar & boost::serialization::make_nvp("perms", perms);

			if (Archive::is_loading::value)
				status = std::filesystem::file_status(
					static_cast<std::filesystem::file_type>(type),
					static_cast<std::filesystem::perms>(perms)
				);
		}

		template<class Archive>
		void serialize(Archive &ar, std::filesystem::path &path, unsigned int version)
		{
			std::filesystem::path::string_type p;

			if (Archive::is_saving::value)
				p = path.native();

			ar & boost::serialization::make_nvp("native", p);

			if (Archive::is_loading::value)
				path = std::filesystem::path(p);
		}
	}
}

namespace cnc {
	namespace common {
		std::string to_string(const std::filesystem::path &path);
		std::filesystem::path path_from_string(const std::string &str);
	}
}