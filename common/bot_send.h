#pragma once
#include "bot_protocol.h"
#include "mac_addr.h"
#include "session.h"
#include "header.h"
#include <boost/asio/ip/tcp.hpp>
#include <filesystem>

namespace cnc { namespace common {
	struct bot_send
	{
		using session_type = session<bot_protocol::magic_byte, bot_protocol::types, boost::asio::ip::tcp::socket>;
		using header_type = session_type::header_type;
		using socket_type = session_type::socket_type;

		template<class T>
		using awaitable_type = session_type::awaitable_type<T>;

		struct err_or_empty_ok_result
		{
			bool err;
			std::string err_msg;
		};

		struct err_or_ok_result : err_or_empty_ok_result
		{
			std::string msg;
		};

		typedef err_or_empty_ok_result hello_result;
		static awaitable_type<hello_result> hello(socket_type &socket, const bot_protocol::hello_data &data);

		typedef err_or_empty_ok_result connect_result;
		static awaitable_type<connect_result> connect(socket_type &socket, const bot_protocol::connect_data &data);

		typedef err_or_empty_ok_result recv_file_result;
		static awaitable_type<recv_file_result> recv_file(socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size);

		typedef err_or_empty_ok_result send_file_result;
		static awaitable_type<send_file_result> send_file(socket_type &socket, const std::filesystem::path &path, std::ostream &out);

		typedef err_or_empty_ok_result quit_result;
		static awaitable_type<quit_result> quit(socket_type &socket);
		static awaitable_type<quit_result> quit(socket_type &socket, const std::string &msg);
		
		static awaitable_type<err_or_empty_ok_result> recv_err_or_empty_ok(socket_type &socket);
		static awaitable_type<err_or_ok_result> recv_err_or_ok(socket_type &socket);
	};
} }