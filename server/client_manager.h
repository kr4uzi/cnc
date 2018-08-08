#pragma once
#include <common/server_client_session.h>
#include <list>
#include <exception>

namespace cnc { namespace server {
	class potential_client : protected common::server::client::session
	{
		bool m_initialized = false;
		
	protected:
		protocol::hello_data m_hello_data;

	public:
		boost::signals2::signal<void(std::exception_ptr)> on_error;
		using session::on_close;

	public:
		potential_client(boost::asio::ip::tcp::socket socket);
		potential_client(potential_client &&) = default;
		potential_client &operator=(potential_client &&) = default;

		const protocol::hello_data &get_hello_data() const;
		const common::mac_addr &get_mac_addr() const;

		void initialize(boost::asio::yield_context yield);
		using session::close;
	};

	class client : protected potential_client
	{
	private:
		bool m_running = false;

	public:
		client(potential_client session);
		client(client &&) = default;
		client &operator=(client &&) = default;

		boost::signals2::signal<void(const std::string &)> on_quit;

		void run(boost::asio::yield_context yield);
		void stop();
	};

	class client_manager
	{
		boost::asio::io_context &m_context;
		boost::asio::ip::tcp::acceptor m_acceptor;
		bool m_running = false;
		std::list<potential_client> m_potentials;
		std::list<client> m_clients;

	public:
		boost::signals2::signal<void(std::exception_ptr)> on_error;

	public:
		client_manager(boost::asio::io_context &context);
		~client_manager();

		void run();
		void stop();

	private:
		void accept(boost::asio::yield_context yield);
	};
} }
