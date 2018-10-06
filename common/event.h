#pragma once
#include <experimental/coroutine>
#include <vector>

namespace cnc { namespace common {
	class event
	{
		class awaiter
		{
			event *m_event;

		public:
			awaiter() noexcept;
			explicit awaiter(const event &event) noexcept;
			awaiter(const awaiter &rhs) noexcept;

			bool await_ready() const noexcept { return m_event == nullptr; }
			bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept;
			void await_resumse() const noexcept { }
		};

	public:
		event(bool set = false) noexcept;
		~event();

		awaiter operator co_await() const noexcept;

		void set() noexcept;
		void reset() noexcept;
	};
} }