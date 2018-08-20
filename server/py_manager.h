#pragma once
#include "singleton.h"
#include <common/task.h>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace cnc { namespace server {
	class py_manager : public singleton<py_manager>
	{
		bool m_running = false;
		boost::asio::io_context &m_context;
		std::map<std::string, std::vector<pybind11::function>> m_handlers;

	private:
		friend void pybind11_init_host(pybind11::module &m);

		void registerHandler(const std::string &name, pybind11::function func);
		void unregisterHandler(const std::string &name, pybind11::function func);

	public:
		py_manager(boost::asio::io_context &context);
		~py_manager();

		common::task<void> run(std::chrono::steady_clock::duration clock, std::chrono::steady_clock::duration time);
		void stop();
		bool running() const { return m_running; }

		// call callbacks that previously registered with host.registerHandler("name", func)
		template<typename ...Args>
		void handle(const std::string &handler_name, Args ...args)
		{
			pybind11::gil_scoped_acquire scope;
			for (auto &func : m_handlers[handler_name])
				func(args...);
		}
	};
} }
