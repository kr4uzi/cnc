#include "client_manager.h"
#include "command_client_manager.h"
#include <boost/asio/signal_set.hpp>
#include <iostream>
using namespace cnc::server;

int main()
{
	boost::asio::io_context context;
	client_manager client_mgr{ context };
	command_client_manager command_mgr{ context };
	boost::asio::signal_set signals{ context };

	auto client_task = client_mgr.run();
	auto command_task = command_mgr.run();
	signals.async_wait([&](auto error, auto signal)
	{
		command_mgr.stop();
		command_task.wait();

		client_mgr.stop();
		client_task.wait();
	});
	
	context.run();
}