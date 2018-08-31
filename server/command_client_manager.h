#pragma once
#include <common/task.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <list>
#include <vector>

namespace cnc {
	namespace server {
		class potential_command_client;
		class command_client;

		struct command_client_data
		{
			boost::asio::ip::address ip;
		};

		class command_client_manager
		{
			bool m_running = false;
			bool m_stopping = false;
			boost::asio::io_context &m_context;
			boost::asio::ip::tcp::acceptor m_acceptor;
			std::list<command_client> m_clients;

		public:
			command_client_manager(boost::asio::io_context &context);
			~command_client_manager();

			common::task<void> run();
			void stop();
			bool stopping() const { return m_stopping; }

			std::vector<command_client_data> data() const;
		};
	}
}
