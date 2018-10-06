#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <common/task.h>
#include <common/mac_addr.h>
#include <list>
#include <vector>

namespace cnc { namespace server {
	class client;
	struct client_data
	{
		boost::asio::ip::address ip;
		common::mac_addr mac;
	};

	class client_manager
	{
		boost::asio::io_context &m_context;
		boost::asio::ip::tcp::acceptor m_acceptor;
		bool m_running = false;
		bool m_stopping = false;
		std::list<client> m_clients;

	public:
		client_manager(boost::asio::io_context &context);
		~client_manager();

		common::task<void> run();
		void stop();
		bool stopping() const { return m_stopping; }

		const std::list<client> &clients() const { return m_clients; }
	};
} }