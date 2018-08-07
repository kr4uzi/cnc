#include "config.h"
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>

using namespace cnc::common;

config::parse_error config::parse(const std::string& filename, config &cfg)
{
	std::ifstream in;
	in.open(filename, std::ios::in);

	if (in.fail())
		return parse_error("error opening file");

	std::string line;
	while (!in.eof())
	{
		std::getline(in, line);

		// ignore black lines and comments
		if (line.empty() || line[0] == '#')
			continue;

		// remove newlines and normalize end-line-character
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

		auto split = line.find('=');
		if (split == std::string::npos)
			continue;

		auto key_start = line.find_first_not_of(" ");
		auto key_end = line.find(" ", key_start);
		auto value_start = line.find_first_not_of(" ", split + 1);
		auto value_end = line.size();

		if (value_start != std::string::npos)
		{
			auto key = line.substr(key_start, key_end - key_start);
			auto value = line.substr(value_start, value_end - value_start);
			cfg.m_values[key].push_back(value);
		}
	}

	in.close();
	return parse_error("");
}

bool config::exists(const std::string& key) const
{
	return m_values.find(key) != m_values.end();
}

std::vector<std::string> config::get_array(const std::string &key, const std::vector<std::string>default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return iter->second;
}

std::string config::get_string(const std::string &key, const std::string &default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return iter->second.front();
}

std::uint32_t config::get_uint32(const std::string &key, std::uint32_t default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return std::stoul(iter->second.front());
}

std::uint64_t config::get_uint64(const std::string &key, std::uint64_t default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return std::stoul(iter->second.front());
}

float config::get_float(const std::string &key, float default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return std::stof(iter->second.front());
}

double config::get_double(const std::string &key, double default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return std::stod(iter->second.front());
}

long double config::get_ldouble(const std::string &key, long double default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	return std::stold(iter->second.front());
}

bool config::get_bool(const std::string &key, bool default_value) const
{
	auto iter = m_values.find(key);
	if (iter == m_values.end())
		return default_value;

	auto value = iter->second.front();
	boost::to_lower(value);
	if (value == "true" || value == "1")
		return true;
	else if (value == "false" || value == "0")
		return false;
	else
		throw std::invalid_argument("not a bool type");

	return default_value;
}

void config::set_string(const std::string& key, const std::string &value)
{
	if (exists(key))
		m_values[key].front() = value;
	else
		m_values[key].push_back(value);
}

void config::set_uint32(const std::string& key, std::uint32_t value)
{
	if (exists(key))
		m_values[key].front() = std::to_string(value);
	else
		m_values[key].push_back(std::to_string(value));
}

void config::set_uint64(const std::string& key, std::uint64_t value)
{
	if (exists(key))
		m_values[key].front() = std::to_string(value);
	else
		m_values[key].push_back(std::to_string(value));
}

void config::set_float(const std::string& key, float value)
{
	if (exists(key))
		m_values[key].front() = std::to_string(value);
	else
		m_values[key].push_back(std::to_string(value));
}

void config::set_double(const std::string& key, double value)
{
	if (exists(key))
		m_values[key].front() = std::to_string(value);
	else
		m_values[key].push_back(std::to_string(value));
}

void config::set_ldouble(const std::string& key, long double value)
{
	if (exists(key))
		m_values[key].front() = std::to_string(value);
	else
		m_values[key].push_back(std::to_string(value));
}

void config::set_bool(const std::string& key, bool value)
{
	std::string strvalue = value ? "true" : "false";
	if (exists(key))
		m_values[key].front() = value;
	else
		m_values[key].push_back(strvalue);
}
