#pragma once
#include "observer_client_protocol.h"
#include "session.h"
#include <boost/asio/ip/tcp.hpp>

namespace cnc { namespace common { namespace observer { namespace client {
	struct send
	{
		using session = session<protocol, boost::asio::ip::tcp::socket>;

		struct [[nodiscard]] err_or_empty_ok_result
		{
			bool err;
			std::string err_msg;
		};

		struct [[nodiscard]] err_or_ok_result : err_or_empty_ok_result
		{
			std::string msg;
		};

		using hello_result = err_or_ok_result;
		static task<hello_result> hello(boost::asio::ip::tcp::socket &socket);
		static task<hello_result> hello(boost::asio::ip::tcp::socket &socket, const std::string &msg);

		using create_directory_result = err_or_empty_ok_result;
		static task<create_directory_result> create_directory(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path);

		struct list_directory_result
		{
			bool err;
			std::string err_msg;
			protocol::directory_view view;
		};
		static task<list_directory_result> list_directory(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path);

		using recv_file_result = err_or_empty_ok_result;
		static task<recv_file_result> recv_file(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path, std::istream &in, protocol::header::size_type size);
		using send_file_result = err_or_empty_ok_result;
		static task<send_file_result> send_file(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path, std::ostream &out);

		struct execute_result : err_or_empty_ok_result
		{
			std::string result;
		};
		static task<execute_result> execute(boost::asio::ip::tcp::socket &socket, const std::string &cmd);

		using quit_result = err_or_ok_result;
		static task<quit_result> quit(boost::asio::ip::tcp::socket &socket);
		static task<quit_result> quit(boost::asio::ip::tcp::socket &socket, const std::string &msg);

		static task<err_or_empty_ok_result> recv_err_or_empty_ok(boost::asio::ip::tcp::socket &socket, const protocol::header &header);
		static task<err_or_ok_result> recv_err_or_ok(boost::asio::ip::tcp::socket &socket, const protocol::header &header);
	};
} } } }