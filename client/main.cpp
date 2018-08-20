#include "server_session.h"
#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <string>
#include <regex>
#include <iostream>
using namespace cnc;
using namespace cnc::client;

namespace cnc { namespace common {
template<class CharT>
void validate(boost::any &value, const std::vector<std::basic_string<CharT>> &values, mac_addr *, int)
{
	// this allows a mixture of : and - but this is actually allowed by the IEEE standard
	static std::regex exp{ R"REGEX(^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$)REGEX" };

	using namespace boost::program_options;

	validators::check_first_occurrence(value);
	auto &str = validators::get_single_string(values);

	if (!std::regex_match(str, exp))
		throw validation_error(validation_error::kind_t::invalid_option_value);

	mac_addr addr;
	for (int i = 0; i < 6; i++)
	{
		auto pos = 3*i;
		addr[i] = ((str[pos] - '0') << 4) | (str[pos + 1] - '0');
	}

	value = addr;
}
} }

namespace boost { namespace asio { namespace ip {
template<class CharT>
void validate(boost::any &value, const std::vector<std::basic_string<CharT>> &values, address *, int)
{
	using namespace boost::program_options;

	validators::check_first_occurrence(value);
	auto &str = validators::get_single_string(values);

	boost::system::error_code err;
	auto ip = make_address(str, err);
	if (err)
		throw validation_error(validation_error::kind_t::invalid_option_value);

	value = ip;
}
} } }

int main(int argc, char *argv[])
{
	namespace po = boost::program_options;

	boost::asio::ip::address ip;
	unsigned short port;
	common::mac_addr mac;
		
	auto po_mac = po::value<common::mac_addr>(&mac)->required();
	auto addrs = common::get_mac_addresses();
	if (!addrs.empty())
		po_mac = po_mac->default_value(addrs.front(), common::to_string(addrs.front()));

	po::options_description desc{ "Allowed options:" };
	desc.add_options()
		("help", "produces this help message")
		("ip", po::value<boost::asio::ip::address>(&ip)->required()->default_value(boost::asio::ip::make_address("192.168.178.26")), "sets the ip address")
		("port", po::value<unsigned short>(&port)->required()->default_value(server::session::protocol::tcp_port), "sets the port")
		("mac", po_mac, "sets the mac address")
		;
	
	try
	{
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			std::cout << desc << '\n';
			return 0;
		}
	}
	catch (po::unknown_option &e)
	{
		std::cout << e.what() << '\n'
			<< desc << '\n';

		return 1;
	}
	catch (po::error &e)
	{
		std::cout << e.what() << '\n';
		return 1;
	}
	
	boost::asio::io_context context;
	boost::asio::signal_set signals{ context, SIGINT };
	server::session session{ context };
	
	auto session_task = [&]() -> std::future<void>
	{
		try
		{
			std::stringstream connect_msg;
			connect_msg << "connecting to " << ip << ":" << port << " with mac " << common::to_string(mac);

			std::cout << connect_msg.str() << " ...\n";
			co_await session.connect({ ip, port }, mac);
			std::cout << connect_msg.str() << "connection successfull\n";

			auto result = co_await session.listen();
			if (result)
			{
				std::cout << "The connection has been ended by the server: " << *result << "\n";
				co_return;
			}

			// no return value = no longer running = stopped e.g. by CTRL + C
		}
		catch (std::exception &e)
		{
			std::cout << "exception occurred: " << e.what() << "\n";
		}
		catch (...)
		{
			std::cout << "unknown exception occurred\n";
		}

		session.stop();
		session.close();
	}();

	signals.async_wait([&](auto error, auto signal)
	{
		std::cout << "exiting ...\r";
		[&]() -> std::future<void>
		{
			session.stop();
			co_await session_task;
			std::cout << "exited\n";
		}();
	});
	
	context.run();
	return 0;
}