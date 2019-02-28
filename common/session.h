#pragma once
#include "header.h"
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/experimental.hpp>
#include <cstdint>
#include <array>
#include <string>
#include <stdexcept>
#include <istream>
#include <ostream>

namespace cnc { namespace common {
	//void set_timeout(boost::asio::ip::tcp::socket &socket, unsigned int ms);

	template<
		std::uint8_t MagicByte, 
		class MessageType,
		class SocketType
	>
	struct session
	{
	public:
		static constexpr std::uint8_t magic_byte = MagicByte;
		using header_type = header<MessageType>;
		using socket_type = typename SocketType;

		template<class T>
		using awaitable_type = boost::asio::experimental::awaitable<T>;

		static awaitable_type<header_type> recv_header(socket_type &socket)
		{
			typename header_type::bytearray buffer;
			auto token = co_await boost::asio::experimental::this_coro::token();
			co_await boost::asio::async_read(socket, boost::asio::buffer(buffer), token);
			header_type incoming{ buffer };
			if (incoming.magic_byte() != magic_byte)
			{
				std::runtime_error e("unexpected magic_byte received");
				throw e;
			}

			co_return std::move(incoming);
		}

		static awaitable_type<void> send_msg(socket_type &socket, typename header_type::message_type type)
		{
			co_await send_msg(socket, type, "");
		}

		static awaitable_type<void> send_msg(socket_type &socket, typename header_type::message_type type, const std::string &msg)
		{
			header header(magic_byte, type, msg.length());
			auto header_bytes = header.to_bytearray();
			
			std::vector<std::uint8_t> bytes;
			bytes.reserve(header_bytes.size() + msg.size());
			bytes.insert(bytes.end(), header_bytes.begin(), header_bytes.end());
			bytes.insert(bytes.end(), msg.begin(), msg.end());

			auto token = co_await boost::asio::experimental::this_coro::token();
			co_await boost::asio::async_write(socket, boost::asio::buffer(bytes), token);
			co_return;
		}

		static awaitable_type<std::string> recv_msg(socket_type &socket, typename header_type::size_type size)
		{
			if (size >= std::string::npos)
			{
				std::range_error e("message too large to fit into std::string");
				throw e;
			}

			std::string msg;
			msg.resize(size);

			auto token = co_await boost::asio::experimental::this_coro::token();
			co_await boost::asio::async_read(socket, boost::asio::buffer(msg), token);
			co_return msg;
		}

		static awaitable_type<void> send_stream(socket_type &socket, typename header_type::message_type type, std::istream &in, typename header_type::size_type size)
		{
			header header(magic_byte, type, size);
			auto buffer = header.to_bytearray();

			auto token = co_await boost::asio::experimental::this_coro::token();
			co_await boost::asio::async_write(socket, boost::asio::buffer(buffer), token);

			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min(size, buffer.max_size());
				in.read(buffer.data(), bytes);
				size -= bytes;
				co_await boost::asio::async_write(socket, boost::asio::buffer(buffer.data(), bytes), token);
			}
		}

		static awaitable_type<void> recv_stream(socket_type &socket, std::ostream &out, typename header_type::size_type size)
		{
			auto token = co_await boost::asio::experimental::this_coro::token();

			while (size)
			{
				std::array<char, 4096> buffer;
				auto bytes = std::min<decltype(size)>(buffer.max_size(), size);
				co_await boost::asio::async_read(socket, boost::asio::buffer(buffer.data(), bytes), token);
				out.write(buffer.data(), bytes);
				size -= bytes;
			}
		}
	};
} }