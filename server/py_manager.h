#pragma once
#include "singleton.h"
#include <common/task.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

namespace cnc { namespace server {
	class client_manager;
	class command_client_manager;

	class py_manager : public singleton<py_manager>
	{
		bool m_running = false;
		boost::asio::io_context &m_context;
		client_manager &m_clients;
		command_client_manager &m_commanders;
		std::map<std::string, std::vector<pybind11::function>> m_handlers;

	private:
		friend void pybind11_init_host(pybind11::module &m);

		void registerHandler(const std::string &name, pybind11::function func);
		void unregisterHandler(const std::string &name, pybind11::function func);

		void sendMessageToClient(const boost::asio::ip::address &addr, const std::string &msg, pybind11::function callback);
		void sendMessageToCommander(const boost::asio::ip::address &addr, const std::string &msg, pybind11::function callback);

	public:
		py_manager(boost::asio::io_context &context, client_manager &client_mgr, command_client_manager &command_client_mgr);
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
