#include "client_manager.h"
#include "command_client_manager.h"
#include "py_manager.h"
#include <boost/asio/signal_set.hpp>
#include <iostream>
using namespace cnc::server;

int main()
{
	boost::asio::io_context context;
	client_manager client_mgr(context);
	command_client_manager command_mgr(context);
	py_manager py_mgr(context);
	boost::asio::signal_set signals(context);

	auto client_task = client_mgr.run();
	auto command_task = command_mgr.run();

	using namespace std::literals;
	auto py_task = py_mgr.run(100ms, 20ms);

	signals.async_wait([&](auto error, auto signal)
	{
		static bool stopping;
		if (stopping)
			return;

		stopping = true;
		[&]() -> std::future<void>
		{
			py_mgr.stop();
			co_await py_task;

			command_mgr.stop();
			co_await command_task;

			client_mgr.stop();
			co_await client_task;
		}();
	});
	
	context.run();
}