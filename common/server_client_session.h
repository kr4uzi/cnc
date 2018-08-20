#pragma once
#include "mac_addr.h"
#include "basic_session.h"
#include "server_client_protocol.h"
#include <filesystem>

namespace cnc { namespace common { namespace server { namespace client { namespace send {
	struct session : protected basic_session<protocol>
	{
		using protocol = protocol;

		session(boost::asio::ip::tcp::socket socket);
		session(session &&) = default;
		session &operator=(session &&) = default;

		struct [[nodiscard]] err_or_empty_ok_result
		{
			bool err;
			std::string err_msg;
		};

		struct [[nodiscard]] err_or_ok_result : err_or_empty_ok_result
		{
			std::string msg;
		};

		using hello_result = err_or_empty_ok_result;
		[[nodiscard]]
		task<hello_result> hello(const protocol::hello_data &data);

		using connect_result = err_or_empty_ok_result;
		[[nodiscard]]
		task<connect_result> connect(const protocol::connect_data &data);

		using recv_file_result = err_or_empty_ok_result;
		[[nodiscard]]
		task<recv_file_result> recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size);
		using send_file_result = err_or_empty_ok_result;
		[[nodiscard]]
		task<send_file_result> send_file(const std::filesystem::path &path, std::ostream &out);

		using quit_result = err_or_empty_ok_result;
		[[nodiscard]]
		task<quit_result> quit();
		[[nodiscard]]
		task<quit_result> quit(const std::string &msg);

		[[nodiscard]]
		task<err_or_empty_ok_result> recv_err_or_empty_ok(const protocol::header &header);
		[[nodiscard]]
		task<err_or_ok_result> recv_err_or_ok(const protocol::header &header);
	};
} } } } }