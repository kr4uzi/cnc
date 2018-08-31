#include "py_manager.h"
#include "client_manager.h"
#include "command_client_manager.h"
#include <boost/asio/steady_timer.hpp>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <common/async_timer.h>
#include <thread>
#include <array>
#include <algorithm>
using namespace cnc;
using namespace cnc::server;
namespace py = pybind11;

namespace cnc { namespace server {
PYBIND11_EMBEDDED_MODULE(host, m)
{
	m.def("registerHandler", [](const std::string &name, py::function func)
	{
		py_manager::instance().registerHandler(name, func);
	});

	m.def("unregisterHandler", [](const std::string &name, py::function func)
	{
		py_manager::instance().unregisterHandler(name, func);
	});

	m.def("clientMgr_isValidIndex", [](unsigned int i)
	{

	});
}
} }

singleton<py_manager> *singleton<py_manager>::m_instance;
py_manager::py_manager(boost::asio::io_context &context, client_manager &client_mgr, command_client_manager &command_client_mgr)
	: m_context(context), m_clients(client_mgr), m_commanders(command_client_mgr)
{

}

py_manager::~py_manager()
{

}

void py_manager::registerHandler(const std::string &name, py::function func)
{
	m_handlers[name].push_back(func);
}

void py_manager::unregisterHandler(const std::string &name, py::function func)
{
	auto &handlers = m_handlers[name];
	handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [&func](const auto &obj) { return func.is(obj); }), handlers.end());
}

common::task<void> py_manager::run(std::chrono::steady_clock::duration clock, std::chrono::steady_clock::duration time)
{
	if (m_running)
		throw std::runtime_error("already running");

	std::cout << "running python at a frequency of 1/" << clock.count() << " ns and a runtime of " << time.count << " ns\n";

	m_running = true;
	py::scoped_interpreter interpreter{};

	//py::gil_scoped_acquire scope;
	auto sys = py::module::import("sys");
	sys.attr("path") = std::array<std::string, 3>{ ".", "python", "python37.zip" };
	sys.attr("dont_write_bytecode") = true;

	try
	{
		std::cout << "importing main cnc module python/cnc\n";
		py::module::import("cnc");
	}
	catch (py::error_already_set &e)
	{
		std::cout << "failed to import, exception:\n"
			<< e.what() << '\n';

		co_return;
	}
	
	while (m_running)
	{
		boost::asio::steady_timer timer{ m_context, clock };
		co_await common::async_wait(timer);

		py::gil_scoped_release scope;
		std::this_thread::sleep_for(time);
	}
}

void py_manager::stop()
{
	m_running = false;
}