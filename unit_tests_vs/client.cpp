#include "stdafx.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using cnc::client::protocol;
using session = cnc::client::session;
using types = protocol::types;

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
	inline std::wstring ToString(const types &type)
	{
		switch (type)
		{
		case types::CREATE_DIRECTORY: return L"CREATE_DIRECTORY";
		case types::ERR: return L"ERR";
		case types::EXECUTE: return L"EXECUTE";
		case types::BLOB: return L"BLOB";
		case types::HELLO: return L"HELLO";
		case types::LAST_UNUSED_MEMBER: return L"LAST_UNUSED_MEMBER";
		case types::LIST_DIRECTORY: return L"LIST_DIRECTORY";
		case types::OK: return L"OK";
		case types::QUIT: return L"QUIT";
		case types::RECV_FILE: return L"RECV_FILE";
		case types::SEND_FILE: return L"SEND_FILE";
		}

		throw std::runtime_error("unknown client_protocol type");
	}
} } }

namespace common { namespace client {
	TEST_CLASS(protocol)
	{
		cnc::client::protocol::directory_view view;

		bool equal(const decltype(view) &lhs, const decltype(view) &rhs)
		{
			return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto &lhs, const auto &rhs)
			{
				return std::get<0>(lhs) == std::get<0>(rhs) &&
					std::get<1>(lhs).type() == std::get<1>(rhs).type() &&
					std::get<1>(lhs).permissions() == std::get<1>(rhs).permissions();
			});
		}

	public:
		protocol()
		{
			namespace fs = std::filesystem;
			for (unsigned i = 0; i < 128; i++)
				view.push_back({ fs::path("folder" + std::to_string(i)), fs::file_status(fs::file_type::directory) });

			for (unsigned i = 0; i < 128; i++)
				view.push_back({ fs::path("file" + std::to_string(i)), fs::file_status(fs::file_type::regular) });
		}

		TEST_METHOD(header_encode_decode)
		{
			using types_t = std::underlying_type<types>::type;

			for (types_t type = 0; type < static_cast<types_t>(types::LAST_UNUSED_MEMBER); type++)
			{
				for (std::uint64_t size_log = 0; size_log < 64; size_log++)
				{
					std::uint64_t size = 1ull << size_log;

					::protocol::header encoded(static_cast<types>(type), size);
					Assert::IsTrue(encoded.is_valid());
					Assert::AreEqual(encoded.get_payload_size(), size);
					Assert::AreEqual(static_cast<types_t>(encoded.get_type()), type);

					auto bytes = encoded.to_bytearray();
					::protocol::header decoded(bytes);
					Assert::IsTrue(decoded.is_valid());
					Assert::AreEqual(decoded.get_payload_size(), size);
					Assert::AreEqual(static_cast<types_t>(decoded.get_type()), type);
				}
			}
		}

		TEST_METHOD(directory_view_serialize)
		{
			namespace fs = std::filesystem;

			auto msg = ::protocol::to_string(view);
			decltype(view) view2 = ::protocol::directory_view_from_string(msg);
			Assert::IsTrue(equal(view, view2));
		}
	};

	TEST_CLASS(session)
	{
		cnc::client::protocol::directory_view view;
		std::string file_content;
		boost::asio::io_context ctx;
		tcp::acceptor acceptor;

		bool equal(const decltype(view) &lhs, const decltype(view) &rhs)
		{
			return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto &lhs, const auto &rhs)
			{
				return std::get<0>(lhs) == std::get<0>(rhs) &&
					std::get<1>(lhs).type() == std::get<1>(rhs).type() &&
					std::get<1>(lhs).permissions() == std::get<1>(rhs).permissions();
			});
		}

	public:
		session()
			: acceptor(ctx, tcp::endpoint(tcp::v4(), ::protocol::tcp_port))
		{
			namespace fs = std::filesystem;
			for (unsigned i = 0; i < 128; i++)
				view.push_back({ fs::path("folder" + std::to_string(i)), fs::file_status(fs::file_type::directory) });

			for (unsigned i = 0; i < 128; i++)
				view.push_back({ fs::path("file" + std::to_string(i)), fs::file_status(fs::file_type::regular) });

			for (std::uint64_t i = 0; i < 8500 * 1024; i++)
				file_content += std::to_string(i);
		}

		std::string create_directory_path = R"(C:\my path with spaces\subdir)";
		TEST_METHOD(create_directory)
		{
			namespace ph = std::placeholders;			
			boost::asio::spawn(ctx, std::bind(&session::create_directory_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::create_directory_send, this, ph::_1));
			ctx.run();
		}

		void create_directory_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::CREATE_DIRECTORY);
			Assert::AreEqual(sess.recv_msg(header.get_payload_size(), yield), create_directory_path);
			sess.send_msg(types::OK, yield);
		}

		void create_directory_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto result = sess.create_directory(create_directory_path, yield);
			Assert::IsFalse(result.err);
		}

		std::string list_directory_path = create_directory_path;
		TEST_METHOD(list_directory)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&session::list_directory_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::list_directory_send, this, ph::_1));
			ctx.run();
		}

		void list_directory_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::LIST_DIRECTORY);
			Assert::AreEqual(sess.recv_msg(header.get_payload_size(), yield), list_directory_path);
			sess.send_msg(types::OK, ::protocol::to_string(view), yield);
		}

		void list_directory_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto result = sess.list_directory(create_directory_path, yield);
			Assert::IsFalse(result.err);
			Assert::IsTrue(equal(view, result.view));
		}

		std::filesystem::path send_file_path = R"(C:\some\file\path\file.txt)";
		TEST_METHOD(send_file)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&session::send_file_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::send_file_send, this, ph::_1));
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
			boost::asio::spawn(ctx, std::bind(&session::recv_file_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::recv_file_send, this, ph::_1));
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

		std::string exec_command = "quit";
		std::string exec_result = "quit result";
		TEST_METHOD(execute)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&session::quit_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::quit_send, this, ph::_1));
			ctx.run();
		}

		void execute_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::QUIT);
			Assert::AreEqual(sess.recv_msg(header.get_payload_size(), yield), quit_message);
			sess.send_msg(types::OK, exec_result, yield);
		}

		void execute_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto result = sess.execute(exec_command, yield);
			Assert::IsFalse(result.err);
			Assert::AreEqual(result.result, exec_result);
		}

		std::string hello_message = "hello world";
		TEST_METHOD(hello)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&session::hello_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::hello_send, this, ph::_1));
			ctx.run();
		}

		void hello_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::HELLO);
			Assert::AreEqual(sess.recv_msg(header.get_payload_size(), yield), hello_message);
			sess.send_msg(types::OK, yield);
		}

		void hello_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto result = sess.hello(hello_message, yield);
			Assert::IsFalse(result.err);
		}

		std::string quit_message = "quit";
		TEST_METHOD(quit)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&session::quit_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&session::quit_send, this, ph::_1));
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
			auto result = sess.quit(quit_message, yield);
			Assert::IsFalse(result.err);
		}

		::session accept(boost::asio::yield_context yield_ctx)
		{
			tcp::socket socket(ctx);
			acceptor.async_accept(socket, yield_ctx);
			return ::session(std::move(socket));
		}

		::session connect(yield_context yield)
		{
			tcp::socket socket{ ctx };
			socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), ::protocol::tcp_port), yield);
			return ::session(std::move(socket));
		}
	};
} }