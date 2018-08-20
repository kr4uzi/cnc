#pragma once
#include "singleton.h"
#include <common/task.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <list>

namespace cnc {
	namespace server {
		class potential_command_client;
		class command_client;

		class command_client_manager : public singleton<command_client_manager>
		{
			bool m_running = false;
			boost::asio::io_context &m_context;
			boost::asio::ip::tcp::acceptor m_acceptor;
			std::list<command_client> m_clients;

		public:
			command_client_manager(boost::asio::io_context &context);
			~command_client_manager();

			common::task<void> run();
			void stop();
		};
	}
}
