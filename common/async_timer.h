#pragma once
#include <experimental/coroutine>
#include <boost/asio/steady_timer.hpp>

namespace cnc { namespace common {
	auto async_wait(boost::asio::steady_timer &timer)
	{
		struct [[nodiscard]] Awaitable
		{
			boost::asio::steady_timer &timer;
			boost::system::error_code m_error_code;

			bool await_ready() { return false; }
			void await_suspend(std::experimental::coroutine_handle<> handle)
			{
				timer.async_wait([this, handle](auto error)
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

		return Awaitable{ timer };
	}
} }