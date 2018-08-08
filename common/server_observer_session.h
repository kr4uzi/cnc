#pragma once
#include "basic_session.h"
#include "server_observer_protocol.h"
#include <filesystem>

namespace cnc {
	namespace common {
		namespace server {
			namespace observer {
				struct session : basic_session<protocol>
				{
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

					struct hello_result
					{
						bool err;
						std::string err_msg;
						protocol::clients clients;
					};
					hello_result hello(boost::asio::yield_context yield);

					using recv_file_result = err_or_empty_ok_result;
					recv_file_result recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size, boost::asio::yield_context ctx);
					using send_file_result = err_or_empty_ok_result;
					send_file_result send_file(const std::filesystem::path &path, std::ostream &out, boost::asio::yield_context ctx);

					struct observe_result
					{
						bool err;
						std::string err_msg;
						protocol::logs logs;
					};
					observe_result observe(const mac_addr &data, boost::asio::yield_context yield);
					using unobserve_result = err_or_empty_ok_result;
					unobserve_result unobserve(const mac_addr &mac, boost::asio::yield_context yield);

					using connect_result = err_or_empty_ok_result;
					connect_result connect(const protocol::connect_data &data, boost::asio::yield_context yield);

					using quit_result = err_or_empty_ok_result;
					quit_result quit(const std::string &message, boost::asio::yield_context yield);
					quit_result quit(boost::asio::yield_context yield);

					err_or_empty_ok_result recv_err_or_empty_ok(const protocol::header &header, boost::asio::yield_context yield);
					err_or_ok_result recv_err_or_ok(const protocol::header &header, boost::asio::yield_context yield);
				};
			}
		}
	}
}