#include "stdafx.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using cnc::server::client::protocol;
using types = protocol::types;
using session = cnc::server::client::session;

bool operator==(const protocol::connect_request &lhs, const protocol::connect_request &rhs)
{
	return lhs.target_ip == rhs.target_ip &&
		lhs.target_port == rhs.target_port;
}

bool operator==(const protocol::hello_data &lhs, const protocol::hello_data &rhs)
{
	return lhs.mac == rhs.mac;
}

namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework {
			inline std::wstring ToString(const types &type)
			{
				switch (type)
				{
				case types::OK: return L"OK";
				case types::ERR: return L"ERR";
				case types::RECV_FILE: return L"RECV_FILE";
				case types::SEND_FILE: return L"SEND_FILE";
				case types::BLOB: return L"BLOB";
				case types::CONNECT: return L"CONNECT";
				case types::QUIT: return L"QUIT";
				case types::LAST_UNUSED_MEMBER: return L"LAST_UNUSED_MEMBER";
				}

				throw std::runtime_error("unknown type");
			}
		}
	}
}

namespace common {
	namespace server {
		TEST_CLASS(client_session)
		{
			boost::asio::io_context ctx;
			tcp::acceptor acceptor;
			std::string file_content;

		public:
			client_session()
				: acceptor(ctx, tcp::endpoint(tcp::v4(), session::protocol::tcp_port))
			{
				for (std::uint64_t i = 0; i < 8500 * 1024; i++)
					file_content += std::to_string(i);
			}

			std::filesystem::path send_file_path = R"(C:\some\file\path\file.txt)";
			TEST_METHOD(send_file)
			{
				namespace ph = std::placeholders;
				boost::asio::spawn(ctx, std::bind(&client_session::send_file_recv, this, ph::_1));
				boost::asio::spawn(ctx, std::bind(&client_session::send_file_send, this, ph::_1));
				ctx.run();
			}

			void send_file_recv(yield_context yield)
			{
				auto sess = accept(yield);
				auto header = sess.recv_header(yield);

				Assert::AreEqual(header.get_type(), types::SEND_FILE);
				Assert::AreEqual(cnc::path_from_string(sess.recv_msg(header.get_payload_size(), yield)), send_file_path);

				sess.send_msg(types::OK, yield);
				std::stringstream ss;
				ss << file_content;
				Assert::AreEqual(ss.str().size(), file_content.size());
				sess.send_stream(types::BLOB, ss, file_content.length(), yield);
			}

			void send_file_send(yield_context yield)
			{
				auto sess = connect(yield);
				std::stringstream ss;
				auto result = sess.send_file(send_file_path, ss, yield);
				Assert::IsFalse(result.err);
				Assert::AreEqual(ss.str(), file_content);
			}

			std::filesystem::path recv_file_path = R"(C:\some\file\path\file.txt)";
			TEST_METHOD(rev_file)
			{
				namespace ph = std::placeholders;
				boost::asio::spawn(ctx, std::bind(&client_session::recv_file_recv, this, ph::_1));
				boost::asio::spawn(ctx, std::bind(&client_session::recv_file_send, this, ph::_1));
				ctx.run();
			}

			void recv_file_recv(yield_context yield)
			{
				auto sess = accept(yield);
				{
					auto header = sess.recv_header(yield);
					Assert::AreEqual(header.get_type(), types::RECV_FILE);
					Assert::AreEqual(cnc::path_from_string(sess.recv_msg(header.get_payload_size(), yield)), recv_file_path);
					sess.send_msg(types::OK, yield);
				}
				{
					auto header = sess.recv_header(yield);
					Assert::AreEqual(header.get_type(), types::BLOB);
					std::stringstream ss;
					sess.recv_stream(ss, header.get_payload_size(), yield);
					Assert::AreEqual(ss.str().size(), file_content.size());
				}
			}

			void recv_file_send(yield_context yield)
			{
				auto sess = connect(yield);
				std::stringstream ss;
				ss << file_content;
				auto result = sess.recv_file(recv_file_path, ss, file_content.size(), yield);
				Assert::IsFalse(result.err);
			}

			protocol::hello_data hello_data = {
				{ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 }
			};
			TEST_METHOD(hello)
			{
				namespace ph = std::placeholders;
				boost::asio::spawn(ctx, std::bind(&client_session::hello_recv, this, ph::_1));
				boost::asio::spawn(ctx, std::bind(&client_session::hello_send, this, ph::_1));
				ctx.run();
			}

			void hello_recv(yield_context yield)
			{
				auto sess = accept(yield);
				auto header = sess.recv_header(yield);
				Assert::AreEqual(header.get_type(), types::HELLO);
				auto data = protocol::hello_data_from_string(sess.recv_msg(header.get_size(), yield));
				Assert::AreEqual(hello_data, data);
				sess.send_msg(types::OK, yield);
			}

			void hello_send(yield_context yield)
			{
				auto sess = connect(yield);
				auto response = sess.hello(hello_data, yield);
				Assert::IsFalse(response.err);
			}

			std::string quit_message = "quit message";
			TEST_METHOD(quit)
			{
				namespace ph = std::placeholders;
				boost::asio::spawn(ctx, std::bind(&client_session::quit_recv, this, ph::_1));
				boost::asio::spawn(ctx, std::bind(&client_session::quit_send, this, ph::_1));
				ctx.run();
			}

			void quit_recv(yield_context yield)
			{
				auto sess = accept(yield);
				auto header = sess.recv_header(yield);
				Assert::AreEqual(header.get_type(), types::QUIT);
				Assert::AreEqual(sess.recv_msg(header.get_payload_size(), yield), quit_message);
				sess.send_msg(types::OK, yield);
			}

			void quit_send(yield_context yield)
			{
				auto sess = connect(yield);
				auto response = sess.quit(quit_message, yield);
				Assert::IsFalse(response.err);
			}

			protocol::connect_request conn_req = {
				boost::asio::ip::address::from_string("127.12.3.4"),
				12345
			};
			TEST_METHOD(connect)
			{
				namespace ph = std::placeholders;
				boost::asio::spawn(ctx, std::bind(&client_session::connect_recv, this, ph::_1));
				boost::asio::spawn(ctx, std::bind(&client_session::connect_send, this, ph::_1));
				ctx.run();
			}

			void connect_recv(yield_context yield)
			{
				auto sess = accept(yield);
				auto header = sess.recv_header(yield);
				Assert::AreEqual(header.get_type(), types::CONNECT);
				auto req = protocol::connect_request_from_string(sess.recv_msg(header.get_size(), yield));
				Assert::AreEqual(conn_req, req);
				sess.send_msg(types::OK, yield);
			}

			void connect_send(yield_context yield)
			{
				auto sess = connect(yield);
				auto response = sess.connect(conn_req, yield);
				Assert::IsFalse(response.err);
			}

			session accept(boost::asio::yield_context yield_ctx)
			{
				tcp::socket socket(ctx);
				acceptor.async_accept(socket, yield_ctx);
				return session(std::move(socket));
			}

			session connect(yield_context yield)
			{
				tcp::socket socket{ ctx };
				socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), session::protocol::tcp_port), yield);
				return session(std::move(socket));
			}
		};
	}
}