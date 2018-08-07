#include "stdafx.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using cnc::server::observer::protocol;
using types = protocol::types;
using session = cnc::server::observer::session;

bool operator==(const protocol::request_connect &lhs, const protocol::request_connect &rhs)
{
	return lhs.dest_ip == rhs.dest_ip &&
		lhs.dest_port == rhs.dest_port &&
		lhs.target_mac == rhs.target_mac;
}

bool operator==(const protocol::log &lhs, const protocol::log &rhs)
{
	return lhs.mac == rhs.mac &&
		lhs.msg == rhs.msg &&
		lhs.time == rhs.time &&
		lhs.type == rhs.type;
}

bool operator==(const protocol::client_data &lhs, const protocol::client_data &rhs)
{
	return lhs.hello_data.mac == rhs.hello_data.mac &&
		lhs.ip == rhs.ip;
}

bool operator==(const protocol::clients &lhs, const protocol::clients &rhs)
{
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto &x, const auto &y) { return x == y; });
}

bool operator==(const protocol::logs &lhs, const protocol::logs &rhs)
{
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto &x, const auto &y) { return x == y; });
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
				case types::REGISTERED: return L"REGISTERED";
				case types::UNREGISTERED: return L"UNREGISTERED";
				case types::OBSERVE: return L"OBSERVE";
				case types::LOG: return L"LOG";
				case types::UNOBSERVE: return L"UNOBSERVE";
				case types::CONNECT: return L"CONNECT";
				case types::QUIT: return L"QUIT";
				case types::LAST_UNUSED_MEMBER: return L"LAST_UNUSED_MEMBER";
				}

				throw std::runtime_error("unknown type");
			}
		}
	}
}

namespace common { namespace server {
	::protocol::clients clients = {
		{ { 0x19, 0x28, 0x32, 0x41, 0x54, 0x63 }, boost::asio::ip::address::from_string("127.0.0.1") },
		{ { 0x79, 0x88, 0x92, 0x10, 0x11, 0x12 }, boost::asio::ip::address::from_string("127.0.0.2") },
		{ { 0x12, 0x23, 0x34, 0x45, 0x51, 0x62 }, boost::asio::ip::address::from_string("127.0.0.3") },
		{ { 0x71, 0x83, 0x95, 0x10, 0x11, 0x12 }, boost::asio::ip::address::from_string("127.0.0.4") }
	};

	cnc::mac_addr log_mac = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };
	::protocol::logs logs = {
		{ log_mac, 0, "d", "e" },
		{ log_mac, 0, "c", "f" },
		{ log_mac, 0, "b", "g" },
		{ log_mac, 0, "a", "h" }
	};

	::protocol::request_connect req_conn = {
		{ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 },
		boost::asio::ip::address::from_string("127.0.0.1"),
		12345
	};

	TEST_CLASS(protocol)
	{
		TEST_METHOD(client_data_serialize)
		{
			auto client_str = ::protocol::to_string(clients[0]);
			auto client_deserialized = ::protocol::client_data_from_string(client_str);
			Assert::AreEqual(clients[0], client_deserialized);
		}

		TEST_METHOD(clients_serialize)
		{
			auto clients_str = ::protocol::to_string(clients);
			auto clients_deserialized = ::protocol::clients_from_string(clients_str);
			bool eq = clients == clients_deserialized;
			Assert::AreEqual(clients, clients_deserialized);
		}

		TEST_METHOD(logs_serialize)
		{
			auto logs_str = ::protocol::to_string(logs);
			auto logs_deserialized = ::protocol::logs_from_string(logs_str);
			Assert::AreEqual(logs, logs_deserialized);
		}

		::protocol::log log = { { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 }, 0, "d", "e" };
		TEST_METHOD(log_serialize)
		{
			auto log_str = ::protocol::to_string(log);
			auto log_deserialized = ::protocol::log_from_string(log_str);
			Assert::AreEqual(log, log_deserialized);
		}

		TEST_METHOD(request_connect_serialize)
		{
			auto req_str = ::protocol::to_string(req_conn);
			auto req_deserialized = ::protocol::request_connect_from_string(req_str);
			Assert::AreEqual(req_conn, req_deserialized);
		}
	};

	TEST_CLASS(observer_session)
	{
		boost::asio::io_context ctx;
		tcp::acceptor acceptor;

	public:
		observer_session()
			: acceptor(ctx, tcp::endpoint(tcp::v4(), session::protocol::tcp_port))
		{

		}

		TEST_METHOD(hello)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&observer_session::hello_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&observer_session::hello_send, this, ph::_1));
			ctx.run();
		}

		void hello_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::HELLO);
			Assert::IsTrue(header.get_payload_size() == 0);			
			sess.send_msg(types::OK, session::protocol::to_string(clients), yield);
		}

		void hello_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto response = sess.hello(yield);
			Assert::IsFalse(response.err);
			Assert::AreEqual(clients, response.clients);
		}

		cnc::mac_addr observe_addr = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };
		TEST_METHOD(observe)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&observer_session::observe_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&observer_session::observe_send, this, ph::_1));
			ctx.run();
		}

		void observe_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::OBSERVE);
			Assert::AreEqual(observe_addr, cnc::mac_addr_from_string(sess.recv_msg(header.get_payload_size(), yield)));
			sess.send_msg(types::OK, ::protocol::to_string(logs), yield);
		}

		void observe_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto response = sess.observe(observe_addr, yield);
			Assert::IsFalse(response.err);
			Assert::AreEqual(logs, response.logs);
		}

		std::string quit_message = "quit message";
		TEST_METHOD(quit)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&observer_session::quit_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&observer_session::quit_send, this, ph::_1));
			ctx.run();
		}

		void quit_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::QUIT);
			Assert::AreEqual(quit_message, sess.recv_msg(header.get_payload_size(), yield));
			sess.send_msg(types::OK, yield);
		}

		void quit_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto response = sess.quit(quit_message, yield);
			Assert::IsFalse(response.err);
		}

		TEST_METHOD(unobserve)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&observer_session::unobserve_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&observer_session::unobserve_send, this, ph::_1));
			ctx.run();
		}

		void unobserve_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::UNOBSERVE);
			Assert::AreEqual(observe_addr, cnc::mac_addr_from_string(sess.recv_msg(header.get_payload_size(), yield)));
			sess.send_msg(types::OK, yield);
		}

		void unobserve_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto response = sess.unobserve(observe_addr, yield);
			Assert::IsFalse(response.err);
		}

		TEST_METHOD(connect)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&observer_session::connect_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&observer_session::connect_send, this, ph::_1));
			ctx.run();
		}

		void connect_recv(yield_context yield)
		{
			auto sess = accept(yield);
			auto header = sess.recv_header(yield);
			Assert::AreEqual(header.get_type(), types::CONNECT);
			Assert::IsFalse(header.get_payload_size() == 0);
			auto req = session::protocol::request_connect_from_string(sess.recv_msg(header.get_payload_size(), yield));
			Assert::IsTrue(req.target_mac == req_conn.target_mac);
			Assert::IsTrue(req.dest_ip == req_conn.dest_ip);
			Assert::IsTrue(req.dest_port == req_conn.dest_port);
			sess.send_msg(types::OK, yield);
		}

		void connect_send(yield_context yield)
		{
			auto sess = connect(yield);
			auto response = sess.connect(req_conn, yield);
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
} }