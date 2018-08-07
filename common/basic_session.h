#pragma once
#include "header.h"
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/signals2/signal.hpp>
#include <string>
#include <vector>
#include <utility>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace cnc {
	namespace common {
		void set_timeout(boost::asio::ip::tcp::socket &socket, unsigned int ms);

		template<class S>
		class session_error : public std::runtime_error
		{
		private:
			S & m_session;

		public:
			session_error(S &session, const std::string &msg = "")
				: std::runtime_error(msg), m_session(session)
			{

			}

			S &session() { return m_session; }
		};

		template<class S>
		class unexpected_message_error : public session_error<S>
		{
			typename S::protocol::header m_header;

		public:
			unexpected_message_error(S & session, const typename S::protocol::header &header, const std::string &msg = "")
				: session_error<S>(session, msg), m_header(header)
			{

			}

			typename S::protocol::header get_header() const { return m_header; }
		};

		template<typename Protocol>
		class basic_session
		{
		public:
			using protocol = typename Protocol;
			using session_error = session_error<basic_session<Protocol>>;
			using unexpected_message_error = unexpected_message_error<basic_session<Protocol>>;

		private:
			bool m_closed = false;
			bool m_closing = false;

		protected:
			boost::asio::ip::tcp::socket m_socket;

		public:
			boost::signals2::signal<void()> on_close;

			basic_session(boost::asio::ip::tcp::socket socket, unsigned int timeout = 1000)
				: m_socket(std::move(socket))
			{
				set_timeout(m_socket, timeout);
			}

			basic_session(basic_session &&) = default;
			basic_session &operator=(basic_session &) = default;

			~basic_session()
			{
				close();
			}

			void close()
			{
				if (m_closed)
					return;

				m_closing = true;
				m_socket.shutdown(boost::asio::socket_base::shutdown_both);
				m_socket.close();

				m_closing = false;
				m_closed = true;
				on_close();
			}

			bool is_closed() const { return m_closed; }
			bool is_closing() const { return m_closing; }

			typename protocol::header recv_header(boost::asio::yield_context yield_ctx)
			{
				std::array<std::uint8_t, protocol::header::get_size()> buffer;
				boost::asio::async_read(m_socket, boost::asio::buffer(buffer), yield_ctx);
				return protocol::header(buffer);
			}

			void send_msg(typename protocol::types type, boost::asio::yield_context yield_ctx)
			{
				send_msg(type, "", yield_ctx);
			}

			void send_msg(typename protocol::types type, const std::string &msg, boost::asio::yield_context yield_ctx)
			{
				typename protocol::header header(type, msg.length());
				auto buffer = header.to_bytearray();
				std::vector<unsigned char> data;
				data.insert(data.end(), buffer.begin(), buffer.end());
				data.insert(data.end(), msg.begin(), msg.end());

				boost::asio::async_write(m_socket, boost::asio::buffer(data), yield_ctx);
			}

			std::string recv_msg(std::uint64_t size, boost::asio::yield_context yield_ctx)
			{
				if (size >= std::string::npos)
					throw new std::length_error("the expected message cannot fit into std::string");

				std::string msg;
				msg.resize(static_cast<std::size_t>(size));

				boost::asio::async_read(m_socket, boost::asio::buffer(msg), yield_ctx);
				return msg;
			}

			void send_stream(typename protocol::types type, std::istream &in, std::uint64_t size, boost::asio::yield_context yield_ctx)
			{
				typename protocol::header header(type, size);
				auto buffer = header.to_bytearray();
				boost::asio::async_write(m_socket, boost::asio::buffer(buffer), yield_ctx);

				while (size)
				{
					std::array<char, 4096> buffer;
					auto bytes = std::min<decltype(size)>(size, buffer.max_size());
					in.read(buffer.data(), bytes);
					size -= bytes;
					boost::asio::async_write(m_socket, boost::asio::buffer(buffer, bytes), yield_ctx);
				}
			}

			void recv_stream(std::ostream &out, std::uint64_t size, boost::asio::yield_context yield_ctx)
			{
				while (size)
				{
					std::array<char, 4096> buffer;
					auto bytes = std::min<decltype(size)>(buffer.max_size(), size);
					boost::asio::async_read(m_socket, boost::asio::buffer(buffer.data(), bytes), yield_ctx);
					out.write(buffer.data(), bytes);
					size -= bytes;
				}
			}
		};
	}
}