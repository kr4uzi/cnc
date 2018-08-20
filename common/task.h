#pragma once
#include <future>

namespace cnc { namespace common {
	template<typename T>
	using task = std::future<T>;
} }
