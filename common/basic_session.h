#pragma once
#include "header.h"
#include "task.h"
#include "async_net.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <array>
#include <string>
#include <vector>
#include <utility>
#include <istream>
#include <ostream>

namespace cnc { namespace common {
	void set_timeout(boost::asio::ip::tcp::socket &socket, unsigned int ms);	

	template<class S>
	class session_error : public std::runtime_error
	{
	private:
		S& m_session;

	public:
		session_error(S&session, std::string const& msg = "")
			: std::runtime_error(msg), m_session(session)
		{

		}

		S& session() { return m_session; }
		S const& session() const { return m_session; }
	};

	template<class S, class H>
	class unexpected_message_error : public session_error<S>
	{
		H m_header;

	public:
		unexpected_message_error(S & session, H const& header, const std::string &msg = "")
			: session_error<S>(session, msg), m_header(header)
		{

		}

		H header() const { return m_header; }
	};

	template<typename Protocol, typename socket_type_t = boost::asio::ip::tcp::socket>
	class basic_session
	{
	public:
		using socket_type = typename socket_type_t;
		using protocol = typename Protocol;
		using session_error = session_error<basic_session<Protocol>>;
		using unexpected_message_error = unexpected_message_error<basic_session<Protocol>, typename protocol::header>;

	private:
		bool m_closed = false;

	protected:
		socket_type m_socket;

	public:
		basic_session(socket_type socket, unsigned int timeout = 1000)
			: m_socket(std::move(socket))
		{
			set_timeout(m_socket, timeout);
		}

		basic_session(basic_session &&rhs)
			: m_closed(rhs.m_closed), m_socket(std::move(rhs.m_socket))
		{				
			rhs.m_closed = true;
		}

		~basic_session()
		{
			close();
		}

		void close()
		{
			if (m_closed)
				return;

			m_socket.shutdown(boost::asio::socket_base::shutdown_both);
			m_socket.close();
			m_closed = true;
		}

		bool closed() const { return m_closed; }

		[[nodiscard]]
		task<typename protocol::header> recv_header()
		{
			std::array<std::uint8_t, protocol::header::get_size()> buffer;
			co_await async_read(m_socket, boost::asio::buffer(buffer));
			co_return protocol::header(buffer);
		}

		[[nodiscard]]
		task<void> send_msg(typename protocol::types type)
		{
			co_await send_msg(type, "");
		}

		[[nodiscard]]
		task<void> send_msg(typename protocol::types type, const std::string &msg)
		{
			typename protocol::header header(type, msg.length());
			auto buffer = header.to_bytearray();
			std::vector<unsigned char> data;
			data.insert(data.end(), buffer.begin(), buffer.end());
			data.insert(data.end(), msg.begin(), msg.end());

			co_await async_write(m_socket, boost::asio::buffer(data));
		}

		[[nodiscard]]
		task<std::string> recv_msg(std::string::size_type size)
		{
			if (size >= std::string::npos)
				throw new std::length_error("the expected message cannot fit into std::string");

			std::string msg;
			msg.resize(size);

			co_await async_read(m_socket, boost::asio::buffer(msg));
			co_return msg;
		}

		[[nodiscard]]
		task<void> send_stream(typename protocol::types type, std::istream &in, typename protocol::header::size_type size)
		{
			typename protocol::header header(type, size);
			auto buffer = header.to_bytearray();
			co_await async_write(m_socket, boost::asio::buffer(buffer));

			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min<decltype(size)>(size, buffer.max_size());
				in.read(buffer.data(), bytes);
				size -= bytes;
				co_await async_write(m_socket, boost::asio::buffer(buffer, bytes));
			}
		}

		[[nodiscard]]
		task<void> recv_stream(std::ostream &out, typename protocol::header::size_type size)
		{
			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min<decltype(size)>(buffer.max_size(), size);
				co_await async_read(m_socket, boost::asio::buffer(buffer.data(), bytes));
				out.write(buffer.data(), bytes);
				size -= bytes;
			}
		}
	};
} }