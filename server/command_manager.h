#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <stdexcept>

namespace cnc {
	namespace server {
		class command_session
		{
			bool m_running = false;

		protected:
			boost::beast::websocket::stream<boost::asio::ip::tcp> m_socket;

		public:
			command_session(boost::asio::ip::tcp::socket socket);

			void run(boost::asio::yield_context yield);
			void stop();
		};
	}
}