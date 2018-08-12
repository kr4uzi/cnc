#include "mac_addr.h"
#include <sstream>
#include <iomanip>
using namespace cnc;

std::string common::to_string(const mac_addr &addr)
{
	std::ostringstream ss;
	for (mac_addr::size_type i = 0; i < addr.size(); i++)
	{
		ss << std::setw(2) << std::setfill('0') << std::hex << (int)addr[i];
		if (i < (addr.size() - 1))
			ss << ":";
	}

	return ss.str();
}