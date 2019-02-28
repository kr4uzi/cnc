#pragma once
#include "default_deserialize.h"
#include <vector>
#include <filesystem>
#include <ostream>

namespace cnc { namespace common {
	struct cmd_relay_protocol
	{
		enum class types
		{
			// answer types
			OK,					// string
			ERR,				// string

			// request types
			HELLO,				// payload: string, answer: {OK + string}|{ERROR + string}
			RECV_FILE,			// payload: path, answer: OK|{ERROR + string}
			SEND_FILE,			// payload: path, answer: {OK + byte[]}|{ERROR + string}
			BLOB,				// payload: byte[], answer: none
			CREATE_DIRECTORY,	// payload: path, answer: OK|{ERROR + string}
			LIST_DIRECTORY,		// payload: path, answer: {OK + directory_view}|{ERROR + string}
			EXECUTE,			// payload: command, answer: {OK + string}|{ERROR + string}
			QUIT				// payload: none|string, answer: OK|{ERROR + string}
		};

		static constexpr std::uint8_t magic_byte = 0xAB;

		struct directory_entry
		{
			std::filesystem::path path;
			std::filesystem::file_status status;
		};
		typedef std::vector<directory_entry> directory_view;
	};

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, cmd_relay_protocol::types type)
	{
		using types = typename cmd_protocol::types;
		switch (type)
		{
		case types::OK:			return os << "OK";
		case types::ERR:		return os << "ERR";

		case types::HELLO:		return os << "HELLO";
		case types::RECV_FILE:	return os << "RECV_FILE";
		case types::SEND_FILE:	return os << "SEND_FILE";
		case types::BLOB:		return os << "BLOB";
		case types::CREATE_DIRECTORY:	return os << "CREATE_DIRECTORY";
		case types::LIST_DIRECTORY:		return os << "LIST_DIRECTORY";
		case types::EXECUTE:	return os << "EXECUTE";
		case types::QUIT:		return os << "QUIT";
		}

		throw std::runtime_error("unknown cmd_protocol type");
	}

	std::string serialize(const cmd_relay_protocol::directory_view &view);
	template<>
	cmd_relay_protocol::directory_view deserialize<cmd_relay_protocol::directory_view>(const std::string &str);
} }