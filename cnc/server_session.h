#pragma once
#include <common/cmd_send.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/signals2/signal.hpp>
#include <list>

namespace cnc
{
	class server_session
	{
	public:
		template<class T>
		using awaitable_type = common::cmd_send::awaitable_type<T>;

	private:
		boost::asio::ip::tcp::socket m_socket;		
		boost::asio::ip::address m_address;
		unsigned short m_port;
		bool m_stopped = false;
		std::list<awaitable_type<void>> m_tasks;

	public:
		server_session(boost::asio::io_context &context, const boost::asio::ip::address &addr, unsigned short port);
		~server_session();

		awaitable_type<void> run();

		bool stopped() const noexcept { return m_stopped; }
		void stop();

		boost::signals2::signal<void(const common::cmd_protocol::clients &)> on_hello;
		boost::signals2::signal<void(const common::cmd_protocol::client_data &)> on_registered;
		boost::signals2::signal<void(const common::mac_addr &)> on_unregistered;
		boost::signals2::signal<void(const common::cmd_protocol::log &)> on_log;
		boost::signals2::signal<void(const std::string &)> on_quit;
	};
}