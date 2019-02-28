#pragma once
#include <string>
#include <map>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <type_traits>

namespace cnc { namespace common {
	class config
	{
	public:
		class parse_error : public std::runtime_error
		{
			bool m_is_set;

		public:
			parse_error(const std::string& error_message)
				: std::runtime_error(error_message), m_is_set(!error_message.empty())
			{ }

			bool operator!() const { return !m_is_set; }
			operator bool() const { return m_is_set; }
		};

	private:
		std::map<std::string, std::vector<std::string>> m_values;

	public:
		static parse_error parse(const std::string& filename, config &cfg);

		bool exists(const std::string& key) const;

		template<class T, typename = std::enable_if_t<std::is_class_v<T>>>
		T get(const std::string &key, const T &default_value) const
		{
			static_assert(false, "not implemented");
		}

		template<class T, typename = std::enable_if_t<!std::is_class_v<T>>>
		T get(const std::string &key, T defaultValue) const
		{
			static_assert(false, "not implemented");
		}

		template<>
		std::string get(const std::string& key, const std::string &default_value) const;
		template<>
		std::uint32_t get(const std::string& key, std::uint32_t default_value) const;
		template<>
		std::uint64_t get(const std::string& key, std::uint64_t default_value) const;
		template<>
		float get(const std::string& key, float default_value) const;
		template<>
		double get(const std::string& key, double default_value) const;
		template<>
		long double get(const std::string& key, long double default_value) const;
		template<>
		bool get(const std::string& key, bool default_value) const;
		template<>
		std::vector<std::string> get(const std::string &key, const std::vector<std::string> &default_value) const;

		void set(const std::string& key, const std::string &value);
		void set(const std::string& key, std::uint32_t value);
		void set(const std::string& key, std::uint64_t value);
		void set(const std::string& key, float value);
		void set(const std::string& key, double value);
		void set(const std::string& key, long double value);
		void set(const std::string& key, bool value);
	};
} }