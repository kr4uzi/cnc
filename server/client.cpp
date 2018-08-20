#include "client.h"
#include <common/serialize_filesystem.h>
#include <fstream>
#include <limits>

using namespace cnc::common;
using namespace cnc::server;

using boost::asio::ip::tcp;
using cnc::common::server::client::protocol;
using types = protocol::types;

potential_client::potential_client(tcp::socket socket)
	: session(std::move(socket))
{

}

task<protocol::hello_data> potential_client::recv_hello()
{
	auto header = co_await recv_header();
	if (header.get_type() != types::HELLO)
		throw std::runtime_error("unexpected_message");
		//throw unexpected_message_error(*this, header);

	auto payload = co_await recv_msg(header.get_payload_size());
	auto data = deserialize<protocol::hello_data>(payload);
	m_hello_received = true;
	co_return data;
}

task<void> potential_client::reject(const std::string &msg)
{
	if (!m_hello_received)
		throw std::runtime_error("hello not yet received");

	co_await send_msg(types::ERR, msg);
	close();
}

task<void> potential_client::accept()
{
	if (!m_hello_received)
		throw std::runtime_error("hello not yet received");

	co_await send_msg(types::OK);
}

client::client(potential_client session, protocol::hello_data data)
	: potential_client(std::move(session)), m_data(std::move(data))
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
			auto path = deserialize<std::filesystem::path>(co_await recv_msg(header.get_payload_size()));
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
				throw std::runtime_error("unexpected_message");
				//throw unexpected_message_error(*this, blob_header);

			co_await recv_stream(file, header.get_payload_size());
			break;
		}
		case types::SEND_FILE:
		{
			auto path = deserialize<std::filesystem::path>(co_await recv_msg(header.get_payload_size()));
			std::ifstream file(path.native(), std::ios::in | std::ios::binary);
			if (!file)
			{
				co_await send_msg(types::ERR, "unable to open file");
				break;
			}

			auto size = std::filesystem::file_size(path);
			if (size > std::numeric_limits<protocol::header::size_type>::max())
				throw std::range_error("file too big");

			co_await send_stream(types::BLOB, file, static_cast<protocol::header::size_type>(size));
			break;
		}
		case types::QUIT:
		{
			auto msg = co_await recv_msg(header.get_payload_size());
			close();
			break;
		}
		default:
			throw std::runtime_error("unexpected_message");
			//throw unexpected_message_error(*this, header);
		}
	}
}

void client::stop()
{
	m_running = false;
}