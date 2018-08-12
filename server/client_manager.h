#pragma once
#include <common/server_client_session.h>
#include <list>

namespace cnc { namespace server {
	class potential_client : public common::server::client::session
	{
		bool m_initialized = false;
		
	protected:
		protocol::hello_data m_hello_data;

	public:
		potential_client(boost::asio::ip::tcp::socket socket);
		potential_client(potential_client &&) = default;
		potential_client &operator=(potential_client &&) = default;

		const protocol::hello_data &get_hello_data() const;
		const common::mac_addr &get_mac_addr() const;

		common::task<void> initialize();
		using session::close;
	};

	class client : public potential_client
	{
	private:
		bool m_running = false;

	public:
		client(potential_client session);
		client(client &&) = default;
		client &operator=(client &&) = default;

		common::task<void> run();
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
		client_manager(boost::asio::io_context &context);
		~client_manager();

		common::task<void> run();
		void stop();
	};
} }