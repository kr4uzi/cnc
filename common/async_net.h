#pragma once
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <experimental/coroutine>

namespace cnc { namespace common {
	template<typename Socket, typename Endpoint>
	auto async_connect(Socket &socket, Endpoint &endpoint)
	{
		struct [[nodiscard]] Awaitable
		{
			Socket &m_socket;
			Endpoint &m_endpoint;
			boost::system::error_code m_error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				m_socket.async_connect(m_endpoint, [this, handle](auto error)
				{
					m_error_code = error;
					handle.resume();
				});
			}

			void await_resume()
			{
				if (m_error_code)
					throw boost::system::system_error(m_error_code);
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
			boost::system::error_code error_code;

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
					throw boost::system::system_error(error_code);

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
			boost::system::error_code error_code;
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
					throw boost::system::system_error(error_code);

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
			boost::system::error_code error_code;

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
					throw boost::system::system_error(error_code);
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
			boost::system::error_code error_code;

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
					throw boost::system::system_error(error_code);
			}
		};

		return Awaitable{ stream, buffer };
	}
} }
