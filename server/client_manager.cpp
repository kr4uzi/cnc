#include "client_manager.h"
#include "../common/serialize_filesystem.h"
#include <stdexcept>
#include <filesystem>
#include <fstream>
using namespace cnc::server;
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

std::future<void> client_manager::run()
{
	if (m_running)
		return;

	while (m_running)
	{
		try
		{
			tcp::socket socket(m_context);
			co_await m_acceptor.async_accept(socket, boost::asio::use_future);
			auto &sess = m_potentials.emplace_back(std::move(socket));
			auto iter = --m_potentials.end();
			auto close_conn = sess.on_close.connect([this, iter]
			{
				m_potentials.erase(iter);
			});
			auto err_conn = sess.on_error.connect([this, &sess](auto ptr)
			{
				sess.close();
			});

			auto task = [this, &sess, iter, close_conn, err_conn]() -> std::future<void>
			{
				co_await sess.initialize();
				for (auto & session : m_clients)
				{
					if (session.get_mac_addr() == sess.get_mac_addr())
					{
						co_await sess.quit("mac already registered");
						return;
					}
				}

				// signals get moved too, but potential session is no longer valid
				close_conn.disconnect();
				err_conn.disconnect();

				auto &client = m_clients.emplace_back(std::move(sess));
				m_potentials.erase(iter);

				auto new_iter = --m_clients.end();
				client.on_close.connect([this, new_iter]
				{
					m_clients.erase(new_iter);
				});
				client.on_error.connect([this, &client](auto ptr)
				{
					client.close();
					client.stop();
					on_error(ptr);
				});

				co_await client.run();
			};
		}
		catch (...)
		{
			on_error(std::current_exception());
		}
	}
}

potential_client::potential_client(tcp::socket socket)
	: session(std::move(socket))
{

}

std::future<void> potential_client::initialize()
{
	try
	{
		auto header = co_await recv_header();
		if (header.get_type() != types::HELLO)
			throw unexpected_message_error(*this, header);

		auto payload = co_await recv_msg(header.get_payload_size());
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

std::future<void> client::run()
{
	m_running = true;
	while (m_running)
	{
		try
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
			if (!closing())
				on_error(std::current_exception());
		}
	}
}

void client::stop()
{
	m_running = false;
}