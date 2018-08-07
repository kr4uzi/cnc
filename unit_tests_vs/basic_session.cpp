#include "stdafx.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using boost::asio::ip::tcp;
using boost::asio::yield_context;

namespace common {
	struct protocol
	{
		enum class types
		{
			TYPE1,
			TYPE2,
			LAST_UNUSED_MEMBER
		};

		static constexpr std::uint8_t magic_byte = 0x5;
		using header = cnc::header<protocol>;
	};
}

namespace Microsoft {
	namespace VisualStudio {
		namespace CppUnitTestFramework {
			inline std::wstring ToString(const common::protocol::types &type)
			{
				using types = common::protocol::types;
				switch (type)
				{
				case types::TYPE1: return L"TYPE1";
				case types::TYPE2: return L"TYPE2";
				case types::LAST_UNUSED_MEMBER: return L"LAST_UNUSED_MEMBER";
				}

				throw std::runtime_error("unknown type");
			}
		}
	}
}

namespace common {
	using sess = cnc::basic_session<protocol>;
	using types = protocol::types;

	TEST_CLASS(basic_session)
	{
		boost::asio::io_context ctx;
		tcp::acceptor acceptor;

		static constexpr unsigned short port = 5000;

	public:
		basic_session()
			: acceptor(ctx, tcp::endpoint(tcp::v4(), port))
		{

		}

		std::string send_message = "my example message";
		TEST_METHOD(send_msg)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&basic_session::send_msg_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&basic_session::send_msg_send, this, ph::_1));
			ctx.run();
		}

		void send_msg_recv(yield_context yield)
		{
			auto s = accept(yield);
			auto h = s.recv_header(yield);
			Assert::AreEqual(h.get_type(), types::TYPE1);
			Assert::AreEqual(s.recv_msg(h.get_payload_size(), yield), send_message);
		}

		void send_msg_send(yield_context yield)
		{
			auto s = connect(yield);
			s.send_msg(types::TYPE1, send_message, yield);
		}

		TEST_METHOD(send_stream)
		{
			namespace ph = std::placeholders;
			boost::asio::spawn(ctx, std::bind(&basic_session::send_stream_recv, this, ph::_1));
			boost::asio::spawn(ctx, std::bind(&basic_session::send_stream_send, this, ph::_1));
			ctx.run();
		}

		void send_stream_recv(yield_context yield)
		{
			auto s = accept(yield);
			auto h = s.recv_header(yield);
			Assert::AreEqual(h.get_type(), types::TYPE2);
			std::stringstream ss;
			s.recv_stream(ss, h.get_payload_size(), yield);
			Assert::AreEqual(ss.str().size(), send_message.size());
		}

		void send_stream_send(yield_context yield)
		{
			auto s = connect(yield);
			std::stringstream ss;
			ss << send_message;
			s.send_stream(types::TYPE2, ss, send_message.size(), yield);
		}

		sess accept(boost::asio::yield_context yield_ctx)
		{
			tcp::socket socket(ctx);
			acceptor.async_accept(socket, yield_ctx);
			return sess(std::move(socket));
		}

		sess connect(yield_context yield)
		{
			tcp::socket socket{ ctx };
			socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port), yield);
			return sess(std::move(socket));
		}
	};
}