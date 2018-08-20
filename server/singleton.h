#pragma once
#include <stdexcept>
namespace cnc { namespace server {
	template<typename T>
	class singleton
	{
		static singleton<T> *m_instance;

	protected:
		singleton()
		{
			if (m_instance != nullptr)
				throw std::runtime_error("already initialized");

			m_instance = this;
		}

		~singleton()
		{
			m_instance = nullptr;
		}

	public:
		static T &instance()
		{
			if (m_instance == nullptr)
				throw std::runtime_error("not yet initialized");

			return static_cast<T&>(*m_instance);
		}
	};
} }
