#pragma once
#include <common/mac_addr.h>
#include <cpp
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <optional>

namespace cnc { namespace client { namespace server {
	class session
	{
	private:
		bool m_closed = false;
		bool m_listening = false;
		bool m_connecting = false;
		bool m_connected = false;
		bool m_stopping = false;

	protected:
		boost::asio::ip::tcp::socket m_socket;

	public:
		session(boost::asio::io_context &context);
		~session();

		void close();
		bool closed() const noexcept { return m_closed; }

		common::task<void> connect(boost::asio::ip::tcp::endpoint endpoint, const common::mac_addr &addr);
		common::task<std::optional<std::string>> listen();
		void stop() noexcept;
	};
} } }