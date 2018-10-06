#pragma once
#include <common/task.h>
#include <common/event.h>
#include <common/server_client_protocol.h>
#include <boost/asio/ip/tcp.hpp>
#include <queue>
#include <functional>
#include <variant>
#include <string>

namespace cnc { namespace server {
		class potential_client
		{
			bool m_hello_received = false;
			bool m_closed = false;

		protected:
			boost::asio::ip::tcp::socket m_socket;

			using protocol = cnc::common::server::client::protocol;

		public:
			potential_client(boost::asio::ip::tcp::socket socket);
			potential_client(potential_client &&) = default;
			potential_client &operator=(potential_client &&) = default;
			~potential_client();

			common::task<protocol::hello_data> recv_hello();
			common::task<void> reject(const std::string &reason);
			common::task<void> accept();

			boost::asio::ip::address ip() const;

		protected:
			void close();
		};

		class client : public potential_client
		{
		private:
			bool m_running = false;
			bool m_stopping = false;
			bool m_quit = false;
			protocol::hello_data m_data;
			std::queue<event> m_exec_queue;

		public:
			client(potential_client session, protocol::hello_data hello_data);
			client(client &&) = default;
			client &operator=(client &&) = default;

			common::task<void> run();
			void stop();

			bool quit() const { return m_quit; }
			bool stopping() const { return m_stopping; }

			const protocol::hello_data &data() const { return m_data; }
			const common::mac_addr &mac() const { return m_data.mac; }

			common::task<std::string> exec(const std::string &cmd);
		};
	}
}