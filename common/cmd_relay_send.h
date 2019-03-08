#pragma once
#include "bot_protocol.h"
#include "cmd_relay_protocol.h"
#include "session.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/experimental.hpp>

namespace cnc { namespace common {
	struct cmd_relay_send
	{
		using session_type = session<cmd_relay_protocol::magic_byte, cmd_relay_protocol::types, boost::asio::ip::tcp::socket>;
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

		typedef err_or_ok_result hello_result;
		static awaitable_type<hello_result> hello(socket_type &socket, const bot_protocol::hello_data &msg);

		typedef err_or_empty_ok_result create_directory_result;
		static awaitable_type<create_directory_result> create_directory(socket_type &socket, const std::filesystem::path &path);

		struct list_directory_result
		{
			bool err;
			std::string err_msg;
			cmd_relay_protocol::directory_view view;
		};
		static awaitable_type<list_directory_result> list_directory(socket_type &socket, const std::filesystem::path &path);

		typedef err_or_empty_ok_result recv_file_result;
		static awaitable_type<recv_file_result> recv_file(socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size);
		typedef err_or_empty_ok_result send_file_result;
		static awaitable_type<send_file_result> send_file(socket_type &socket, const std::filesystem::path &path, std::ostream &out);

		struct execute_result : err_or_empty_ok_result
		{
			std::string result;
		};
		static awaitable_type<execute_result> execute(socket_type &socket, const std::string &cmd);

		typedef err_or_ok_result quit_result;
		static awaitable_type<quit_result> quit(socket_type &socket);
		static awaitable_type<quit_result> quit(socket_type &socket, const std::string &msg);

		static awaitable_type<err_or_empty_ok_result> recv_err_or_empty_ok(socket_type &socket);
		static awaitable_type<err_or_ok_result> recv_err_or_ok(socket_type &socket);
	};
} }