#pragma once
#include "mac_addr.h"
#include "basic_session.h"
#include "server_client_protocol.h"
#include <filesystem>

namespace cnc {
	namespace common {
		namespace server {
			namespace client {
				class session : public basic_session<protocol>
				{
				public:
					session(boost::asio::ip::tcp::socket socket);
					session(session &&) = default;
					session &operator=(session &&) = default;

					struct err_or_empty_ok_result
					{
						bool err;
						std::string err_msg;
					};

					struct err_or_ok_result
					{
						bool err;
						std::string err_msg;
						std::string msg;
					};

					using hello_result = err_or_empty_ok_result;
					hello_result hello(const protocol::hello_data &data, boost::asio::yield_context yield);

					using connect_result = err_or_empty_ok_result;
					connect_result connect(const protocol::connect_data &data, boost::asio::yield_context yield);

					using recv_file_result = err_or_empty_ok_result;
					recv_file_result recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size, boost::asio::yield_context ctx);
					using send_file_result = err_or_empty_ok_result;
					send_file_result send_file(const std::filesystem::path &path, std::ostream &out, boost::asio::yield_context ctx);

					using quit_result = err_or_empty_ok_result;
					quit_result quit(boost::asio::yield_context yield);
					quit_result quit(const std::string &msg, boost::asio::yield_context yield);

					err_or_empty_ok_result recv_err_or_empty_ok(const protocol::header &header, boost::asio::yield_context yield);
					err_or_ok_result recv_err_or_ok(const protocol::header &header, boost::asio::yield_context yield);
				};
			}
		}
	}
}