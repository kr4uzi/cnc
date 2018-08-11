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
	signals.async_wait([&](auto &err, auto signal)
	{
		client_mgr.stop();
		boost::asio::spawn(context, std::bind(&command_client_manager::stop, std::ref(command_mgr), std::placeholders::_1));
	});

	client_mgr.run();
	boost::asio::spawn(context, std::bind(&command_client_manager::run, std::ref(command_mgr), std::placeholders::_1));

	context.run();
}