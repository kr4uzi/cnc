#ifndef _WIN32
#error "this is a windows only source file"
#endif

#include "mac_addr.h"
#include <WinSock2.h>
#include <IPHlpApi.h>
#include <IPExport.h>
#include <algorithm>
#pragma comment(lib, "Iphlpapi.lib")
using namespace cnc;

std::vector<common::mac_addr> common::get_mac_addresses()
{
	std::vector<std::uint8_t> buffer;
	ULONG buff_len = 4096 * 16;
	auto flags = GAA_FLAG_INCLUDE_PREFIX; // GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
	GetAdaptersAddresses(AF_INET, flags, nullptr, nullptr, &buff_len);
	buffer.resize(buff_len);

	std::vector<common::mac_addr> addrs;
	if (buff_len)
	{
		buffer.resize(buff_len);
		auto ptr = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data());
		auto err = GetAdaptersAddresses(AF_INET, flags, nullptr, ptr, &buff_len);
		if (err == NO_ERROR)
		{
			for (; ptr != nullptr; ptr = ptr->Next)
			{
				if (ptr->PhysicalAddressLength)
				{
					std::vector<std::uint8_t> bytes;
					bytes.resize(ptr->PhysicalAddressLength);
					for (ULONG i = 0; i < ptr->PhysicalAddressLength; i++)
						bytes[i] = ptr->PhysicalAddress[i];

					if (bytes.size() == mac_addr::bytes)
					{
						mac_addr addr;
						std::copy_n(bytes.begin(), bytes.size(), addr.begin());
						addrs.push_back(addr);
					}
				}
			}
		}
	}

	return addrs;
}