#pragma once
#include "singleton.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <common/task.h>
#include <list>

namespace cnc { namespace server {
	class client;
	class client_manager : public singleton<client_manager>
	{
		boost::asio::io_context &m_context;
		boost::asio::ip::tcp::acceptor m_acceptor;
		bool m_running = false;
		std::list<client> m_clients;

	private:
		friend void pybind11_init_client_mgr(pybind11::module &m);

	public:
		client_manager(boost::asio::io_context &context);
		~client_manager();

		common::task<void> run();
		void stop();
	};
} }