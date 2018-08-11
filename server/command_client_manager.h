#pragma once
#include "command_client.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <list>

namespace cnc {
	namespace server {
		class command_client_manager
		{
			bool m_running = false;
			boost::asio::io_context &m_context;
			boost::asio::ip::tcp::acceptor m_acceptor;
			std::list<potential_command_client> m_potentials;
			std::list<command_client> m_clients;

		public:
			command_client_manager(boost::asio::io_context &context);

			void run(boost::asio::yield_context yield);
			void stop(boost::asio::yield_context yield);

			boost::signals2::signal<void(std::exception_ptr)> on_error;

		private:
			void accept(boost::asio::yield_context yield);
		};
	}
}
