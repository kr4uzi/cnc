#pragma once
#include "../common/task.h"
#include "../common/server_observer_protocol.h"
#include <boost/beast/websocket/stream.hpp>
#include <json_spirit/value.h>

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

			[[nodiscard]]
			common::task<void> initialize();
			[[nodiscard]]
			common::task<void> close(boost::beast::websocket::close_code reason);

		protected:
			[[nodiscard]]
			common::task<websocket_message> recv_msg();
		};

		class command_client : public potential_command_client
		{
			bool m_running = false;

		public:
			command_client(potential_command_client client);

			[[nodiscard]]
			common::task<void> run();
			void stop();
		};
	}
}