#pragma once
#include <boost/asio/spawn.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/signals2/signal.hpp>
#include <json_spirit/value.h>
#include <exception>
#include <stdexcept>
#include "../common/server_observer_protocol.h"

namespace cnc {
	namespace server {
		class potential_command_client;
		class command_client_error : public std::runtime_error
		{
			potential_command_client &m_client;

		public:
			command_client_error(potential_command_client &client, const std::string &what = "")
				: std::runtime_error(what), m_client(client)
			{

			}
		};

		struct websocket_message
		{
			cnc::common::server::observer::protocol::types type;
			json_spirit::Value message;
		};

		class invalid_message_error : public command_client_error
		{
			websocket_message &m_message;

		public:
			invalid_message_error(potential_command_client &client, websocket_message &message, const std::string &what = "")
				: command_client_error(client, what), m_message(message)
			{

			}
		};

		class potential_command_client
		{
			bool m_initialized = false;
			bool m_initializing = false;
			bool m_closed = false;
			bool m_closing = false;

		protected:
			boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_socket;

		public:
			potential_command_client(boost::asio::ip::tcp::socket socket);
			potential_command_client(potential_command_client &&) = default;
			potential_command_client &operator=(potential_command_client &&) = default;
			~potential_command_client();

			void initialize(boost::asio::yield_context yield);
			void close(boost::beast::websocket::close_code reason, boost::asio::yield_context yield);

			boost::signals2::signal<void(std::exception_ptr)> on_error;
			boost::signals2::signal<void()> on_close;

		protected:
			websocket_message get_message(boost::asio::yield_context yield);
		};

		class command_client : public potential_command_client
		{
			bool m_running = false;

		public:
			command_client(potential_command_client client);

			void run(boost::asio::yield_context yield);
			void stop();
		};
	}
}