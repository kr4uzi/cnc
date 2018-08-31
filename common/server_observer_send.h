#pragma once
#include "server_observer_protocol.h"
#include "session.h"
#include <filesystem>

namespace cnc { namespace common { namespace server { namespace observer {
	struct send
	{
		using session = session<protocol, boost::asio::ip::tcp::socket>;

		struct[[nodiscard]] err_or_empty_ok_result
		{
			bool err;
			std::string err_msg;
		};

		struct[[nodiscard]] err_or_ok_result : err_or_empty_ok_result
		{
			std::string msg;
		};

		struct[[nodiscard]] hello_result : err_or_empty_ok_result
		{
			protocol::clients clients;
		};
		static task<hello_result> hello(boost::asio::ip::tcp::socket &socket);

		using recv_file_result = err_or_empty_ok_result;
		static task<recv_file_result> recv_file(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path, std::istream &in, protocol::header::size_type size);
		using send_file_result = err_or_empty_ok_result;
		static task<send_file_result> send_file(boost::asio::ip::tcp::socket &socket, const std::filesystem::path &path, std::ostream &out);

		struct observe_result : err_or_empty_ok_result
		{
			protocol::logs logs;
		};
		static task<observe_result> observe(boost::asio::ip::tcp::socket &socket, const mac_addr &data);
		using unobserve_result = err_or_empty_ok_result;
		static task<unobserve_result> unobserve(boost::asio::ip::tcp::socket &socket, const mac_addr &mac);

		using connect_result = err_or_empty_ok_result;
		static task<connect_result> connect(boost::asio::ip::tcp::socket &socket, const protocol::connect_data &data);

		using quit_result = err_or_empty_ok_result;
		static task<quit_result> quit(boost::asio::ip::tcp::socket &socket, const std::string &message);
		static task<quit_result> quit(boost::asio::ip::tcp::socket &socket);

		static task<err_or_empty_ok_result> recv_err_or_empty_ok(boost::asio::ip::tcp::socket &socket, const protocol::header &header);
		static task<err_or_ok_result> recv_err_or_ok(boost::asio::ip::tcp::socket &socket, const protocol::header &header);
	};
} } } }