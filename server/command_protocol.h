#pragma once
#include <ostream>
#include <stdexcept>

namespace cnc {
	namespace server {
		struct command_protocol
		{
			enum class commands
			{
				FIRST_MEMBER_UNUSED,
				HELLO,
				RECV_FILE,
				SEND_FILE,
				BLOB,
				CLIENT,
				CONNECT,
				QUIT,
				LAST_MEMBER_UNUSED
			};
		};

		template<class CharT, class Traits>
		std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, const command_protocol::commands &cmd)
		{
			using cmds = command_protocol::commands;
			switch (cmd)
			{
			case cmds::HELLO:		return os << "HELLO";
			case cmds::RECV_FILE:	return os << "RECV_FILE";
			case cmds::SEND_FILE:	return os << "SEND_FILE";
			case cmds::BLOB:		return os << "BLOB";
			case cmds::CLIENT:		return os << "CLIENT";
			case cmds::CONNECT:		return os << "CONNECT";
			case cmds::QUIT:		return os << "QUIT";
			}

			throw std::runtime_error("unknown command type");
		}
	}
}
