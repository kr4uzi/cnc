#pragma once
#include <exception>
#include <utility>
#include <memory>
#include <variant>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <experimental/coroutine>

namespace common
{
	template<typename T>
	class task;

	class promise_base
	{
		std::experimental::coroutine_handle<> m_waiter;
		std::atomic<bool> m_state;

	public:
		auto initial_suspend() { return std::experimental::suspend_always{}; }

		friend class final_awaitable;
		struct final_awaitable
		{
			bool await_ready() { return false; }
			void await_resume() { }

			template<typename Promise>
			auto await_suspend(std::experimental::coroutine_handle<Promise> coroutine)
			{
				promise_base &promise = coroutine.promise();
				if (promise.m_state.exchange(true, std::memory_order_acq_rel))
					promise.m_waiter.resume();
			}
		};

		auto final_suspend() { return final_awaitable{}; }

		// msvc doesnt support returning a coroutine from await_suspend yet
		// when this is finally supported, the whole state management can be replaced 
		// by returning the waiter in final_awaitable::await_suspend
		bool set_waiter(std::experimental::coroutine_handle<> waiter)
		{
			m_waiter = waiter;
			// return true if we're not yet done
			// this means the calling (await_suspend) function does not have to suspend
			return !m_state.exchange(true, std::memory_order_acq_rel);
		}

	protected:
		bool completed() { return m_state.load(std::memory_order_relaxed); }
	};

	template<typename T>
	class promise : public promise_base
	{
		std::variant<std::monostate, T, std::exception_ptr> result;

	public:
		task<T> get_return_object();

		template<typename U>
		void return_value(U &&value) { result.emplace<1>(std::forward<U>(value)); }
		void unhandled_exception() { result.emplace<2>(std::current_exception()); }

		T &value()
		{
			if (result.index() == 1)
				return std::get<1>(result);

			std::rethrow_exception(std::get<2>(result));
		}
	};

	template<typename T>
	class promise<T&> : public promise_base
	{
		std::variant<std::monostate, T*, std::exception_ptr> result;

	public:
		task<T&> get_return_object();

		void return_value(T &value) { result.emplace<1>(std::addressof(value)); }
		void unhandled_exception() { result.emplace<2>(std::current_exception()); }

		T &value()
		{
			if (result.index() == 1)
				return *std::get<1>(result);

			std::rethrow_exception(std::get<2>(result));
		}
	};

	template<>
	class promise<void> : public promise_base
	{
		std::exception_ptr m_exception;

	public:
		task<void> get_return_object();

		void return_void() { }
		void unhandled_exception() { m_exception = std::current_exception(); }

		void value() { if (m_exception) std::rethrow_exception(m_exception); }
	};

	template<typename T>
	class task
	{
	public:
		using promise_type = promise<T>;
		using handle_type = std::experimental::coroutine_handle<promise_type>;
		using value_type = T;

	private:
		// the coroutine this task wraps
		handle_type m_coroutine{ nullptr };

	public:
		task() = default;
		task(const task &) = delete;
		task &operator=(const task &) = delete;

		task(task &&rhs)
			: m_coroutine(rhs.m_coroutine)
		{
			rhs.m_coroutine = nullptr;
		}

		task &operator=(task &&rhs)
		{
			if (std::addressof(rhs) != this)
			{
				if (m_coroutine)
					m_coroutine.destroy();

				m_coroutine = rhs.m_coroutine;
				rhs.m_coroutine = nullptr;
			}
		}

		explicit task(handle_type coroutine)
			: m_coroutine(coroutine)
		{

		}

		~task()
		{
			if (m_coroutine)
				m_coroutine.destroy();
		}

		bool done() const
		{
			return !m_coroutine || m_coroutine.done();
		}

		void operator()() { m_coroutine.resume(); }

		auto operator co_await()
		{
			struct awaitable
			{
				handle_type &m_coroutine;

				bool await_ready() { return !m_coroutine || m_coroutine.done(); }
				bool await_suspend(std::experimental::coroutine_handle<> awaiting_coroutine)
				{
					m_coroutine.resume();
					return m_coroutine.promise().set_waiter(awaiting_coroutine);
				}

				decltype(auto) await_resume() { return m_coroutine.promise().value(); }
			};

			return awaitable{ m_coroutine };
		}
	};

	template<typename T>
	task<T> promise<T>::get_return_object()
	{
		return task<T>(task<T>::handle_type::from_promise(*this));
	}

	inline task<void> promise<void>::get_return_object()
	{
		return task<void>(task<void>::handle_type::from_promise(*this));
	}

	template<typename T>
	task<T&> promise<T&>::get_return_object()
	{
		return task<T&>(task<T&>::handle_type::from_promise(*this));
	}

	template<typename T>
	auto sync_wait(task<T> &t)
	{
		auto f = t.operator co_await();
		if (!f.await_ready())
		{
			std::mutex mutex;
			std::condition_variable cond;
			bool done = false;

			auto helper_task = [&]() -> task<void>
			{
				std::lock_guard lock(mutex);
				done = true;
				cond.notify_one();
				co_return;
			}();
			auto helper = helper_task.operator co_await();
			f.await_suspend(helper.m_coroutine);
			helper_task();

			std::unique_lock lock(mutex);
			while (!done)
				cond.wait(lock);
		}

		return f.await_resume();
	}
}
