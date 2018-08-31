#pragma once
#include "header.h"
#include "task.h"
#include "async_net.h"
#include <array>
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <boost/asio/ip/tcp.hpp>

namespace cnc { namespace common {
	void set_timeout(boost::asio::ip::tcp::socket &socket, unsigned int ms);	

	template<typename Protocol>
	using session_error = std::runtime_error;

	template<typename Protocol>
	class unexpected_message_error : public session_error<Protocol>
	{
	public:
		using protocol = typename Protocol;

	private:
		typename protocol::header m_header;

	public:
		unexpected_message_error(const typename protocol::header &header, const std::string &msg = "")
			: session_error<protocol>(msg), m_header(header)
		{

		}

		typename protocol::header header() const { return m_header; }
	};

	template<typename Protocol, typename AsyncStream>
	class session
	{
	public:
		using socket_type = typename AsyncStream;
		using protocol = typename Protocol;
		using size_type = typename protocol::header::size_type;
		using session_error = session_error<protocol>;
		using unexpected_message_error = unexpected_message_error<protocol>;

		static task<typename protocol::header> recv_header(socket_type &socket)
		{
			std::array<std::uint8_t, protocol::header::get_size()> buffer;
			co_await async_read(socket, boost::asio::buffer(buffer));
			co_return protocol::header(buffer);
		}

		static task<void> send_msg(socket_type &socket, typename protocol::types type)
		{
			co_await send_msg(socket, type, "");
		}

		static task<void> send_msg(socket_type &socket, typename protocol::types type, const std::string &msg)
		{
			typename protocol::header header(type, msg.length());
			auto buffer = header.to_bytearray();
			std::vector<unsigned char> data;
			data.insert(data.end(), buffer.begin(), buffer.end());
			data.insert(data.end(), msg.begin(), msg.end());
			co_await async_write(socket, boost::asio::buffer(data));
		}

		static task<std::string> recv_msg(socket_type &socket, std::string::size_type size)
		{
			if (size >= std::string::npos)
				throw new std::length_error("the expected message cannot fit into std::string");

			std::string msg;
			msg.resize(size);

			co_await async_read(socket, boost::asio::buffer(msg));
			co_return msg;
		}

		static task<void> send_stream(socket_type &socket, typename protocol::types type, std::istream &in, size_type size)
		{
			typename protocol::header header(type, size);
			auto buffer = header.to_bytearray();
			co_await async_write(socket, boost::asio::buffer(buffer));

			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min<decltype(size)>(size, buffer.max_size());
				in.read(buffer.data(), bytes);
				size -= bytes;
				co_await async_write(socket, boost::asio::buffer(buffer.data(), bytes));
			}
		}

		static task<void> recv_stream(socket_type &socket, std::ostream &out, size_type size)
		{
			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min<decltype(size)>(buffer.max_size(), size);
				co_await async_read(socket, boost::asio::buffer(buffer.data(), bytes));
				out.write(buffer.data(), bytes);
				size -= bytes;
			}
		}
	};
} }