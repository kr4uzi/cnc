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

					struct [[nodiscard]] err_or_empty_ok_result
					{
						bool err;
						std::string err_msg;
					};

					struct [[nodiscard]] err_or_ok_result : err_or_empty_ok_result
					{
						std::string msg;
					};

					struct hello_result : err_or_empty_ok_result
					{
						protocol::clients clients;
					};
					[[nodiscard]]
					std::future<hello_result> hello();

					using recv_file_result = err_or_empty_ok_result;
					[[nodiscard]]
					std::future<recv_file_result> recv_file(const std::filesystem::path &path, std::istream &in, protocol::header::size_type size);
					using send_file_result = err_or_empty_ok_result;
					[[nodiscard]]
					std::future<send_file_result> send_file(const std::filesystem::path &path, std::ostream &out);

					struct observe_result : err_or_empty_ok_result
					{
						protocol::logs logs;
					};
					[[nodiscard]]
					std::future<observe_result> observe(const mac_addr &data);
					using unobserve_result = err_or_empty_ok_result;
					[[nodiscard]]
					std::future<unobserve_result> unobserve(const mac_addr &mac);

					using connect_result = err_or_empty_ok_result;
					[[nodiscard]]
					std::future<connect_result> connect(const protocol::connect_data &data);

					using quit_result = err_or_empty_ok_result;
					[[nodiscard]]
					std::future<quit_result> quit(const std::string &message);
					[[nodiscard]]
					std::future<quit_result> quit();

					[[nodiscard]]
					std::future<err_or_empty_ok_result> recv_err_or_empty_ok(const protocol::header &header);
					[[nodiscard]]
					std::future<err_or_ok_result> recv_err_or_ok(const protocol::header &header);
				};
			}
		}
	}
}