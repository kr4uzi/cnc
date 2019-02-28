#pragma once
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <system_error>
#include <experimental/coroutine>

namespace cnc { namespace common {
	template<typename Socket, typename Endpoint>
	auto async_connect(Socket &socket, Endpoint &endpoint)
	{
		struct [[nodiscard]] Awaitable
		{
			Socket &socket;
			Endpoint &endpoint;
			std::error_code error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				socket.async_connect(endpoint, [this, handle](auto error)
				{
					error_code = error;
					handle.resume();
				});
			}

			void await_resume()
			{
				if (error_code)
					throw std::system_error(error_code);
			}
		};

		return Awaitable{ socket, endpoint };
	}

	template<class Protocol>
	auto async_acccept(boost::asio::basic_socket_acceptor<Protocol> &acceptor)
	{
		struct [[nodiscard]] Awaitable
		{
			decltype(acceptor) &acceptor;
			typename Protocol::socket socket;
			std::error_code error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				acceptor.async_accept(socket, [this, handle](auto error)
				{
					error_code = error;
					handle.resume();
				});
			}

			auto await_resume()
			{
				if (error_code)
					throw std::system_error(error_code);

				return std::move(socket);
			}
		};

		return Awaitable{ acceptor, decltype(Awaitable::socket){ acceptor.get_executor().context() } };
	}

	template<typename AsyncStream, typename BufferSequence>
	auto async_read_some(AsyncStream &stream, BufferSequence const& buffer)
	{
		struct [[nodiscard]] Awaitable
		{
			AsyncStream& stream;
			BufferSequence const& buffer;
			std::error_code error_code;
			std::size_t bytes;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				socket.async_read_some(stream, buffer, [this, handle](auto error, auto len)
				{
					error_code = error;
					bytes = len;
					handle.resume();
				});
			}

			auto await_resume()
			{
				if (error_code)
					throw std::system_error(error_code);

				return bytes;
			}
		};

		return Awaitable{ stream, buffer };
	}

	template <typename AsyncStream, typename BufferSequence>
	auto async_read(AsyncStream &stream, BufferSequence const& buffer)
	{
		struct [[nodiscard]] Awaitable
		{
			AsyncStream& stream;
			BufferSequence const& buffer;
			std::error_code error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				boost::asio::async_read(stream, buffer, [this, handle](auto error, auto bytes)
				{
					error_code = error;
					handle.resume();
				});
			}

			void await_resume()
			{
				if (error_code)
					throw std::system_error(error_code);
			}
		};

		return Awaitable{ stream, buffer };
	}

	template<typename AsyncStream, typename BufferSequence>
	auto async_write(AsyncStream& stream, BufferSequence const& buffer)
	{
		struct [[nodiscard]] Awaitable
		{
			AsyncStream& stream;
			BufferSequence const& buffer;
			std::error_code error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				boost::asio::async_write(stream, buffer, [this, handle](auto error, auto bytes)
				{
					error_code = error;
					handle.resume();
				});
			}

			void await_resume()
			{
				if (error_code)
					throw std::system_error(error_code);
			}
		};

		return Awaitable{ stream, buffer };
	}
} }
