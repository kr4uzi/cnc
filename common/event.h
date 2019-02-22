#pragma once
#include <experimental/coroutine>
#include <vector>

namespace cnc { namespace common {
	class event
	{
		class awaiter
		{
			friend class event;

			event &m_event;
			std::experimental::coroutine_handle<> m_coroutine;

		public:
			explicit awaiter(const event &event) noexcept;
			awaiter(const awaiter &rhs) noexcept;

			bool await_ready() const noexcept { return m_event.is_set(); }
			bool await_suspend(std::experimental::coroutine_handle<> coroutine) noexcept
			{
				m_coroutine = coroutine;
				if (m_event.is_set())
					return true;

				return false;
			}
			void await_resumse() const noexcept { }
		};

		std::vector<awaiter> m_awaiters;
		bool m_set;

	public:
		event(bool set = false) noexcept;
		~event();

		awaiter operator co_await() const noexcept;

		void set() noexcept
		{
			if (m_set)
				return;

			for (auto &awaiter : m_awaiters)
				awaiter.m_coroutine.resume();
		}

		bool is_set() const noexcept { return m_set; }
		void reset() noexcept;
	};
} }