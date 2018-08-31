#pragma once
#include <experimental/coroutine>
#include <future>

namespace cnc { namespace common {
	template<class T>
	using task = std::future<T>;
} }