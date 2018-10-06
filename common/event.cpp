#include "event.h"
using namespace cnc::common;

event::event(bool set) noexcept
	: m_state(set), m_new_waiters(nullptr), m_waiters(nullptr)
{

}

event::~event()
{

}

event::awaiter event::operator co_await() const noexcept
{
	auto old = m_state.load(std::memory_order::memory_order_relaxed);
	if ((old & 0x))
}