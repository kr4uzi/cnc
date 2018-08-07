#include "client_manager.h"
#include "../common/serialize_filesystem.h"
#include <stdexcept>
#include <filesystem>
#include <fstream>
using namespace cnc::server;
using boost::asio::ip::tcp;
using boost::asio::yield_context;
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

void client_manager::run()
{
	if (m_running)
		return;

	boost::asio::spawn(m_context, std::bind(&client_manager::accept, this, std::placeholders::_1));
}

void client_manager::accept(yield_context yield)
{
	while (m_running)
	{
		try
		{
			tcp::socket socket(m_context);
			m_acceptor.async_accept(socket, yield);
			auto &sess = m_potentials.emplace_back(std::move(socket));
			auto iter = --m_potentials.end();
			auto close_conn = sess.on_close.connect([this, iter]
			{
				m_potentials.erase(iter);
			});
			auto err_conn = sess.on_error.connect([this, &sess](std::exception &e)
			{
				sess.close();
			});

			boost::asio::spawn(m_context, [this, &sess, close_conn, err_conn](auto yield)
			{
				sess.initialize(yield);
				for (auto & session : m_clients)
				{
					if (session.get_mac_addr() == sess.get_mac_addr())
					{
						sess.quit("mac already registered", yield);
						return;
					}
				}

				// signals get moved too, but potential session is no longer valid
				close_conn.disconnect();
				err_conn.disconnect();

				auto &client = m_clients.emplace_back(std::move(sess));
				auto iter = --m_clients.end();
				client.on_close.connect([this, iter]
				{
					m_clients.erase(iter);
				});
				client.on_error.connect([this, &client]
				{
					client.close();
				});

				client.run(yield);
			});
		}
		catch (std::exception &e)
		{
			on_error(e);
		}
	}
}

potential_client::potential_client(tcp::socket socket)
	: session(std::move(socket))
{

}

void potential_client::initialize(yield_context yield)
{
	try
	{
		auto header = recv_header(yield);
		if (header.get_type() != types::HELLO)
			throw unexpected_message_error(*this, header);

		auto payload = recv_msg(header.get_payload_size(), yield);
		m_hello_data = protocol::hello_data_from_string(payload);
		m_initialized = true;		
	}
	catch (...)
	{
		on_error(std::current_exception());
	}
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

void client::run(yield_context yield)
{
	m_running = true;
	while (m_running)
	{
		try
		{
			auto header = recv_header(yield);
			switch (header.get_type())
			{
			case types::RECV_FILE:
			{
				auto path = common::path_from_string(recv_msg(header.get_payload_size(), yield));
				if (!std::filesystem::is_directory(path))
				{
					if (!std::filesystem::create_directories(path.parent_path()))
					{
						send_msg(types::ERR, "unable to create directories", yield);
						break;
					}
				}

				std::ofstream file(path.filename(), std::ios::out | std::ios::binary);
				if (!file)
				{
					send_msg(types::ERR, "unable to create file", yield);
					break;
				}

				auto blob_header = recv_header(yield);
				if (blob_header.get_type() != types::BLOB)
					throw unexpected_message_error(*this, blob_header);

				recv_stream(file, header.get_payload_size(), yield);
				break;
			}
			case types::SEND_FILE:
			{
				auto path = common::path_from_string(recv_msg(header.get_payload_size(), yield));
				std::ifstream file(path.native(), std::ios::in | std::ios::binary);
				if (!file)
				{
					send_msg(types::ERR, "unable to open file", yield);
					break;
				}

				send_stream(types::BLOB, file, std::filesystem::file_size(path), yield);
				break;
			}
			case types::QUIT:
			{
				auto msg = recv_msg(header.get_payload_size(), yield);
				on_quit(msg);
				close();
				break;
			}
			default:
				throw unexpected_message_error(*this, header);
			}
		}
		catch (...)
		{
			if (!is_closing())
				on_error(std::current_exception());
		}
	}
}

void client::stop()
{
	m_running = false;
}