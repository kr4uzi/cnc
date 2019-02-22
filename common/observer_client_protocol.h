#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <tuple>

namespace cnc { namespace common { namespace observer { namespace client {
	struct protocol
	{
		enum class types : std::uint8_t
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

		static constexpr unsigned short tcp_port = 5003;
		static constexpr std::uint8_t magic_byte = 0x3;

		using directory_view = std::vector<std::tuple<std::filesystem::path, std::filesystem::file_status>>;
	};

	template<class CharT, class Traits>
	std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const protocol::types &type)
	{
		using types = protocol::types;
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

		throw std::runtime_error("unknown client::protocol type");
	}
} } } }

namespace cnc { namespace common {
	std::string serialize(const observer::client::protocol::directory_view &view);
	template<>
	observer::client::protocol::directory_view deserialize<observer::client::protocol::directory_view>(const std::string &str);
} }