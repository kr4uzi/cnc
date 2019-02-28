#pragma once
#include "session.h"
#include "cmd_protocol.h"
#include <filesystem>
#include <boost/asio/ip/tcp.hpp>

namespace cnc { namespace common {
	struct cmd_send
	{
		using session_type = session<cmd_protocol::magic_byte, cmd_protocol::types, boost::asio::ip::tcp::socket>;
		using socket_type = session_type::socket_type;
		using header_type = session_type::header_type;
		
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

		struct hello_result : err_or_empty_ok_result
		{
			cmd_protocol::clients clients;
		};

		static awaitable_type<hello_result> hello(socket_type &socket);

		typedef err_or_empty_ok_result recv_file_result;
		static awaitable_type<recv_file_result> recv_file(socket_type &socket, const std::filesystem::path &path, std::istream &in, header_type::size_type size);
		typedef err_or_empty_ok_result send_file_result;
		static awaitable_type<send_file_result> send_file(socket_type &socket, const std::filesystem::path &path, std::ostream &out);

		struct observe_result : err_or_empty_ok_result
		{
			cmd_protocol::logs logs;
		};
		static awaitable_type<observe_result> observe(socket_type &socket, const mac_addr &data);
		typedef err_or_empty_ok_result unobserve_result;
		static awaitable_type<unobserve_result> unobserve(socket_type &socket, const mac_addr &mac);

		typedef err_or_empty_ok_result connect_result;
		static awaitable_type<connect_result> connect(socket_type &socket, const cmd_protocol::client_data &data);

		typedef err_or_empty_ok_result quit_result;
		static awaitable_type<quit_result> quit(socket_type &socket, const std::string &message);
		static awaitable_type<quit_result> quit(socket_type &socket);

		static awaitable_type<err_or_empty_ok_result> recv_err_or_empty_ok(socket_type &socket);
		static awaitable_type<err_or_ok_result> recv_err_or_ok(socket_type &socket);
	};
} }