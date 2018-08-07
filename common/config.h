#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <vector>

namespace cnc { namespace common {
	class config
	{
	public:
		class parse_error
		{
		private:
			std::string error_message;

		public:
			parse_error(const std::string& error_message)
				: error_message(error_message)
			{

			}

			std::string what() const { return error_message; }
			bool operator!() const { return error_message.empty(); }
		};

	private:
		std::map<std::string, std::vector<std::string>> m_values;

	public:
		static parse_error parse(const std::string& filename, config &cfg);

		bool exists(const std::string& key) const;

		std::string get_string(const std::string& key, const std::string &default_value) const;
		std::uint32_t get_uint32(const std::string& key, std::uint32_t default_value) const;
		std::uint64_t get_uint64(const std::string& key, std::uint64_t default_value) const;
		float get_float(const std::string& key, float default_value) const;
		double get_double(const std::string& key, double default_value) const;
		long double get_ldouble(const std::string& key, long double default_value) const;
		bool get_bool(const std::string& key, bool default_value) const;
		std::vector<std::string> get_array(const std::string &key, const std::vector<std::string>default_value) const;

		void set_string(const std::string& key, const std::string &value);
		void set_uint32(const std::string& key, std::uint32_t value);
		void set_uint64(const std::string& key, std::uint64_t value);
		void set_float(const std::string& key, float value);
		void set_double(const std::string& key, double value);
		void set_ldouble(const std::string& key, long double value);
		void set_bool(const std::string& key, bool value);
	};
} }