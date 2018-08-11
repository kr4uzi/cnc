#pragma once
#include "client_protocol.h"
#include "basic_session.h"
#include <utility>

namespace cnc {
	namespace common {
		namespace client {
			class session : public basic_session<protocol>
			{
			public:
				session(boost::asio::ip::tcp::socket socket);
				session &operator=(session &&) = default;
				session(session &&) = default;

				struct [[nodiscard]] err_or_empty_ok_result
				{
					bool err;
					std::string err_msg;
				};

				struct[[nodiscard]] err_or_ok_result : err_or_empty_ok_result
				{
					std::string msg;
				};

				using hello_result = err_or_ok_result;
				[[nodiscard]]
				std::future<hello_result> hello();
				[[nodiscard]]
				std::future<hello_result> hello(const std::string &msg);

				using create_directory_result = err_or_empty_ok_result;
				[[nodiscard]]
				std::future<create_directory_result> create_directory(const std::filesystem::path &path);

				struct list_directory_result
				{
					bool err;
					std::string err_msg;
					protocol::directory_view view;
				};
				[[nodiscard]]
				std::future<list_directory_result> list_directory(const std::filesystem::path &path);

				using recv_file_result = err_or_empty_ok_result;
				[[nodiscard]]
				std::future<recv_file_result> recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size);
				using send_file_result = err_or_empty_ok_result;
				[[nodiscard]]
				std::future<send_file_result> send_file(const std::filesystem::path &path, std::ostream &out);

				struct execute_result : err_or_empty_ok_result
				{
					std::string result;
				};
				[[nodiscard]]
				std::future<execute_result> execute(const std::string &cmd);

				using quit_result = err_or_ok_result;
				[[nodiscard]]
				std::future<quit_result> quit();
				[[nodiscard]]
				std::future<quit_result> quit(const std::string &msg);

				[[nodiscard]]
				std::future<err_or_empty_ok_result> recv_err_or_empty_ok(const protocol::header &header);
				[[nodiscard]]
				std::future<err_or_ok_result> recv_err_or_ok(const protocol::header &header);
			};
		}
	}
}