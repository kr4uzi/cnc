#pragma once
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <experimental/coroutine>

namespace cnc {
	namespace common {
		template<class Protocol>
		auto async_acccept(boost::asio::basic_socket_acceptor<Protocol> &acceptor)
		{
			struct Awaitable
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

		template<typename Socket, typename BufferSeq>
		auto async_read_some(Socket &socket, const BufferSeq &buffer)
		{
			struct Awaitable
			{
				Socket &socket;
				const BufferSeq &buffer;
				boost::system::error_code error_code;
				std::size_t bytes;

				bool await_ready() { return false; }
				void await_suspend(std::experimental::coroutine_handle<> handle)
				{
					socket.async_read_some(socket, buffer, [this, handle](auto error, auto len)
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

			return Awaitable{ socket, buffer };
		}

		template<typename Socket, typename BufferSeq>
		auto async_read(Socket &socket, const BufferSeq &buffer)
		{
			struct Awaitable
			{
				Socket &socket;
				const BufferSeq &buffer;
				boost::system::error_code error_code;

				bool await_ready() { return false; }
				void await_suspend(std::experimental::coroutine_handle<> handle)
				{
					boost::asio::async_read(socket, buffer, [this, handle](auto error, auto bytes)
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

			return Awaitable{ socket, buffer };
		}

		template<typename Socket, typename BufferSeq>
		auto async_write(Socket &socket, const BufferSeq &buffer)
		{
			struct Awaitable
			{
				Socket &socket;
				const BufferSeq &buffer;
				boost::system::error_code error_code;

				bool await_ready() { return false; }
				void await_suspend(std::experimental::coroutine_handle<> handle)
				{
					boost::asio::async_write(socket, buffer, [this, handle](auto error, auto bytes)
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

			return Awaitable{ socket, buffer };
		}
	}
}
