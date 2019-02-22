#pragma once
#include <boost/asio/experimental.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cppcoro/task.hpp>

#include <array>
#include <string>
#include <vector>
#include <istream>
#include <ostream>

namespace cnc { namespace common {
	void set_timeout(boost::asio::ip::tcp::socket &socket, unsigned int ms);

	template<typename Protocol>
	using session_error = std::runtime_error;

	template<typename AsyncStream, std::uint8_t MagicByte, typename SizeType = std::size_t>
	class session
	{
	public:
		using socket_type = typename AsyncStream;
		using size_type = typename SizeType;
		using session_error = session_error<protocol>;

		/*static cppcoro::task<typename protocol::header> recv_header(socket_type &socket)
		{
			std::array<std::uint8_t, protocol::header::get_size()> buffer;
			co_await async_read(socket, boost::asio::buffer(buffer));
			co_return protocol::header(buffer);
		}*/

		static auto recv_header(socket_type &socket)
		{
			auto token = co_await boost::asio::experimental::this_coro::token();

			std::array<std::uint8_t, sizeof(MagicByte) + sizeof(SizeType)> buffer;
			boost::asio::
		}

		//static task<void> send_msg(socket_type &socket, typename protocol::types type)
		//{
		//	co_await send_msg(socket, type, "");
		//}

		//static task<void> send_msg(socket_type &socket, typename protocol::types type, const std::string &msg)
		//{
		//	typename protocol::header header(type, msg.length());
		//	auto buffer = header.to_bytearray();
		//	std::vector<unsigned char> data;
		//	data.insert(data.end(), buffer.begin(), buffer.end());
		//	data.insert(data.end(), msg.begin(), msg.end());
		//	co_await async_write(socket, boost::asio::buffer(data));
		//}

		//static task<std::string> recv_msg(socket_type &socket, std::string::size_type size)
		//{
		//	if (size >= std::string::npos)
		//		throw new std::length_error("the expected message cannot fit into std::string");

		//	std::string msg;
		//	msg.resize(size);

		//	co_await async_read(socket, boost::asio::buffer(msg));
		//	co_return msg;
		//}

		//static task<void> send_stream(socket_type &socket, typename protocol::types type, std::istream &in, size_type size)
		//{
		//	typename protocol::header header(type, size);
		//	auto buffer = header.to_bytearray();
		//	co_await async_write(socket, boost::asio::buffer(buffer));

		//	while (size)
		//	{
		//		std::array<char, 4096> buffer;
		//		auto bytes = std::min<decltype(size)>(size, buffer.max_size());
		//		in.read(buffer.data(), bytes);
		//		size -= bytes;
		//		co_await async_write(socket, boost::asio::buffer(buffer.data(), bytes));
		//	}
		//}

		//static task<void> recv_stream(socket_type &socket, std::ostream &out, size_type size)
		//{
		//	while (size)
		//	{
		//		std::array<char, 4096> buffer;
		//		auto bytes = std::min<decltype(size)>(buffer.max_size(), size);
		//		co_await async_read(socket, boost::asio::buffer(buffer.data(), bytes));
		//		out.write(buffer.data(), bytes);
		//		size -= bytes;
		//	}
		//}
	};
} }