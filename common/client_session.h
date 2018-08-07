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

				struct err_or_empty_ok_result
				{
					bool err;
					std::string err_msg;
				};

				struct err_or_ok_result : err_or_empty_ok_result
				{
					std::string msg;
				};

				using hello_result = err_or_ok_result;
				hello_result hello(boost::asio::yield_context yield);
				hello_result hello(const std::string &msg, boost::asio::yield_context yield);

				using create_directory_result = err_or_empty_ok_result;
				create_directory_result create_directory(const std::filesystem::path &path, boost::asio::yield_context yield);

				struct list_directory_result
				{
					bool err;
					std::string err_msg;
					protocol::directory_view view;
				};
				list_directory_result list_directory(const std::filesystem::path &path, boost::asio::yield_context yield);

				using recv_file_result = err_or_empty_ok_result;
				recv_file_result recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size, boost::asio::yield_context yield);
				using send_file_result = err_or_empty_ok_result;
				send_file_result send_file(const std::filesystem::path &path, std::ostream &out, boost::asio::yield_context yield);

				struct execute_result : err_or_empty_ok_result
				{
					std::string result;
				};
				execute_result execute(const std::string &cmd, boost::asio::yield_context yield);

				using quit_result = err_or_ok_result;
				quit_result quit(boost::asio::yield_context yield);
				quit_result quit(const std::string &msg, boost::asio::yield_context yield);

				err_or_empty_ok_result recv_err_or_empty_ok(const protocol::header &header, boost::asio::yield_context yield);
				err_or_ok_result recv_err_or_ok(const protocol::header &header, boost::asio::yield_context yield);
			};
		}
	}
}