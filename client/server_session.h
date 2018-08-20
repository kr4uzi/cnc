#pragma once
#include "../common/server_client_session.h"
#include <optional>

namespace cnc {
	namespace client {
		namespace server {
			class session : protected common::server::client::session
			{
			private:
				bool m_running = false;
				bool m_connecting = false;
				bool m_connected = false;

			public:
				session(boost::asio::io_context &context);

				common::task<void> connect(boost::asio::ip::tcp::endpoint endpoint, const common::mac_addr &addr);
				common::task<std::optional<std::string>> listen();
				void stop() noexcept;

				using common::server::client::session::close;
				using common::server::client::session::protocol;
			};
		}
	}
}