#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/signals2/signal.hpp>
#include <exception>
#include <stdexcept>

namespace cnc {
	namespace server {
		class potential_command_client
		{
			bool m_initialized = false;

		protected:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_socket;

		public:
			potential_command_client(boost::asio::ip::tcp::socket socket);

			void initialize(boost::asio::yield_context yield);
			void close();
		};

		class command_session
		{
			bool m_running = false;

		protected:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_socket;

		public:
			command_session(boost::asio::ip::tcp::socket socket);

			void run(boost::asio::yield_context yield);
			void stop();
		};
	}
}