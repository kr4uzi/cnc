#pragma once
#include <common/server_client_session.h>

namespace cnc {
	namespace server {
		class potential_client : protected common::server::client::send::session
		{
			bool m_hello_received = false;

		public:
			potential_client(boost::asio::ip::tcp::socket socket);
			potential_client(potential_client &&) = default;
			potential_client &operator=(potential_client &&) = default;

			[[nodiscard]]
			common::task<protocol::hello_data> recv_hello();
			[[nodiscard]]
			common::task<void> reject(const std::string &reason);
			[[nodiscard]]
			common::task<void> accept();
		};

		class client : public potential_client
		{
		private:
			bool m_running = false;
			protocol::hello_data m_data;

		public:
			client(potential_client session, protocol::hello_data hello_data);
			client(client &&) = default;
			client &operator=(client &&) = default;

			[[nodiscard]]
			common::task<void> run();
			void stop();

			const protocol::hello_data &data() const { return m_data; }
			const common::mac_addr &mac() const { return m_data.mac; }
		};
	}
}