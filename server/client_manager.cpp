#include "client_manager.h"
#include "../common/serialize_filesystem.h"
#include <boost/log/trivial.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
using namespace cnc::server;
using namespace cnc::common;
using boost::asio::ip::tcp;
using cnc::common::server::client::protocol;
using types = protocol::types;

client_manager::client_manager(boost::asio::io_context &context)
	: m_context(context),
	m_acceptor(context, tcp::endpoint(tcp::v4(), protocol::tcp_port))
{

}

client_manager::~client_manager()
{
	stop();
}

void client_manager::stop()
{
	m_running = false;
	for (auto &client : m_clients)
		client.stop();
}

std::string mac_addr_to_readable_str(const mac_addr &addr)
{
	std::string str;
	for (auto & byte : addr)
	{
		std::ostringstream ss;
		ss << std::hex << std::setw(2) << std::setfill('0') << byte;
		str += ss.str();
	}

	return str;
}

std::future<void> client_manager::run()
{
	if (m_running)
		return;

	std::vector<common::task<void>> tasks;
	m_running = true;

	while (m_running)
	{
		auto socket = co_await common::async_acccept(m_acceptor);
		auto &sess = m_potentials.emplace_back(std::move(socket));
		auto sess_iter = --m_potentials.end();

		auto task = [this, &sess, sess_iter]() -> common::task<void>
		{				
			try
			{
				co_await sess.initialize();
				for (auto & session : m_clients)
				{
					if (session.get_mac_addr() == sess.get_mac_addr())
					{
						co_await sess.quit("mac already registered");
						throw;
					}
				}

				auto &client = m_clients.emplace_back(std::move(sess));
				m_potentials.erase(sess_iter);
				auto &client_iter = --m_clients.end();

				try
				{
					co_await client.run();
				}
				catch (...)
				{
					BOOST_LOG_TRIVIAL(error) << "[ClientMgr][" << mac_addr_to_readable_str(client.get_mac_addr()) << "]: " << 
				}

				// the client has to be erased either way (error or not)
				m_clients.erase(client_iter);
			}
			catch (...)
			{
				m_potentials.erase(sess_iter);
			}
		}();

		tasks.push_back(std::move(task));
	}

	for (auto &task : tasks)
		co_await task;
}

potential_client::potential_client(tcp::socket socket)
	: session(std::move(socket))
{

}

std::future<void> potential_client::initialize()
{
	auto header = co_await recv_header();
	if (header.get_type() != types::HELLO)
		throw unexpected_message_error(*this, header);

	auto payload = co_await recv_msg(header.get_payload_size());
	m_hello_data = protocol::hello_data_from_string(payload);
	m_initialized = true;
}

const protocol::hello_data &potential_client::get_hello_data() const
{
	if (!m_initialized)
		throw std::runtime_error("not initialized");

	return m_hello_data;
}

const cnc::common::mac_addr &potential_client::get_mac_addr() const
{
	return get_hello_data().mac;
}

client::client(potential_client session)
	: potential_client(std::move(session))
{

}

std::future<void> client::run()
{
	m_running = true;
	while (m_running)
	{
		auto header = co_await recv_header();
		switch (header.get_type())
		{
		case types::RECV_FILE:
		{
			auto path = common::path_from_string(co_await recv_msg(header.get_payload_size()));
			if (!std::filesystem::is_directory(path))
			{
				if (!std::filesystem::create_directories(path.parent_path()))
				{
					co_await send_msg(types::ERR, "unable to create directories");
					break;
				}
			}

			std::ofstream file(path.filename(), std::ios::out | std::ios::binary);
			if (!file)
			{
				co_await send_msg(types::ERR, "unable to create file");
				break;
			}

			auto blob_header = co_await recv_header();
			if (blob_header.get_type() != types::BLOB)
				throw unexpected_message_error(*this, blob_header);

			co_await recv_stream(file, header.get_payload_size());
			break;
		}
		case types::SEND_FILE:
		{
			auto path = common::path_from_string(co_await recv_msg(header.get_payload_size()));
			std::ifstream file(path.native(), std::ios::in | std::ios::binary);
			if (!file)
			{
				co_await send_msg(types::ERR, "unable to open file");
				break;
			}

			co_await send_stream(types::BLOB, file, std::filesystem::file_size(path));
			break;
		}
		case types::QUIT:
		{
			auto msg = co_await recv_msg(header.get_payload_size());
			close();
			break;
		}
		default:
			throw unexpected_message_error(*this, header);
		}
	}
}

void client::stop()
{
	m_running = false;
}