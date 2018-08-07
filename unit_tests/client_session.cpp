#include <boost/test/unit_test.hpp>
#include "../common/client_session.h"
#include "../common/serialize_filesystem.h"
#include "client_print.h"
using boost::asio::ip::tcp;
using boost::asio::yield_context;
using cnc::client::protocol;
using cnc::client::session;
using types = protocol::types;

#include <functional>
#include <string>

struct helper
{
	boost::asio::io_context ctx;
	tcp::acceptor acceptor;

	helper()
		: acceptor(ctx, tcp::endpoint(tcp::v4(), ::protocol::tcp_port))
	{

	}

	session accept(boost::asio::yield_context yield_ctx)
	{
		tcp::socket socket(ctx);
		acceptor.async_accept(socket, yield_ctx);
		return ::session(std::move(socket));
	}

	session connect(yield_context yield)
	{
		tcp::socket socket{ ctx };
		socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), ::protocol::tcp_port), yield);
		return ::session(std::move(socket));
	}

	void run()
	{
		ctx.run();
	}

	template<class F>
	void spawn(F &&f)
	{
		boost::asio::spawn(ctx, std::bind(std::forward<F>(f), std::ref(*this), std::placeholders::_1));
	}

	template<class F, class... Args>
	void spawn(F &&f, Args &&...args)
	{
		boost::asio::spawn(ctx, std::bind(std::forward<F>(f), std::forward<Args>(args)..., std::ref(*this), std::placeholders::_1));
	}
};

void create_directory_recv(const std::string &path, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	auto header = sess.recv_header(yield);
	BOOST_TEST_REQUIRE(header.get_type() == types::CREATE_DIRECTORY);
	BOOST_TEST(sess.recv_msg(header.get_payload_size(), yield) == path);
	sess.send_msg(types::OK, yield);
}

void create_directory_send(const std::string &path, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	auto result = sess.create_directory(path, yield);
	BOOST_TEST(!result.err);
}

BOOST_AUTO_TEST_CASE(create_directory)
{
	helper hlp;
	std::string path = R"(C:\my path with spaces\subdir)";

	hlp.spawn(create_directory_recv, path);
	hlp.spawn(create_directory_send, path);
	hlp.run();
}

protocol::directory_view create_view()
{
	::protocol::directory_view view;
	namespace fs = std::filesystem;
	for (unsigned i = 0; i < 128; i++)
		view.push_back({ fs::path("folder" + std::to_string(i)), fs::file_status(fs::file_type::directory) });

	for (unsigned i = 0; i < 128; i++)
		view.push_back({ fs::path("file" + std::to_string(i)), fs::file_status(fs::file_type::regular) });

	return view;
}

void list_directory_recv(const std::string &path, const protocol::directory_view &view, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	auto header = sess.recv_header(yield);
	BOOST_TEST_REQUIRE(header.get_type() == types::LIST_DIRECTORY);
	BOOST_TEST(sess.recv_msg(header.get_payload_size(), yield) == path);
	sess.send_msg(types::OK, ::protocol::to_string(view), yield);
}

void list_directory_send(const std::string &path, const protocol::directory_view &view, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	auto result = sess.list_directory(path, yield);
	BOOST_TEST_REQUIRE(!result.err);
	BOOST_TEST(view == result.view);
}

BOOST_AUTO_TEST_CASE(list_directory)
{
	helper hlp;
	std::string path = R"(C:\my path with spaces\subdir)";
	auto view = create_view();
	hlp.spawn(list_directory_recv, std::cref(path), std::cref(view));
	hlp.spawn(list_directory_send, std::cref(path), std::cref(view));
	hlp.run();
}

std::string create_file_content()
{
	std::string file_content;
	for (std::uint64_t i = 0; i < 8500 * 1024; i++)
		file_content += std::to_string(i);

	return file_content;
}

void send_file_recv_handler(session &sess, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	auto header = sess.recv_header(yield);

	BOOST_TEST_REQUIRE(header.get_type() == types::SEND_FILE);
	BOOST_TEST(cnc::path_from_string(sess.recv_msg(header.get_payload_size(), yield)) == path);

	sess.send_msg(types::OK, yield);
	std::stringstream ss;
	ss << file_content;
	BOOST_TEST(ss.str().size(), file_content.size());
	sess.send_stream(types::BLOB, ss, file_content.length(), yield);
}

void send_file_recv(const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	send_file_recv_handler(sess, path, file_content, hlp, yield);
}

void send_file_send_handler(session &sess, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	std::stringstream ss;
	auto result = sess.send_file(path, ss, yield);
	BOOST_TEST_REQUIRE(!result.err);
	BOOST_TEST(ss.str() == file_content);
}

void send_file_send(const std::filesystem::path &path, const std::string &content, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	send_file_send_handler(sess, path, content, hlp, yield);
}

BOOST_AUTO_TEST_CASE(send_file)
{
	helper hlp;
	std::filesystem::path path = R"(C:\some\file\path\file.txt)";
	auto file_content = create_file_content();
	hlp.spawn(send_file_recv, path, file_content);
	hlp.spawn(send_file_send, path, file_content);
	hlp.run();
}

void execute_recv(const std::string &cmd, const std::string &result, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	auto header = sess.recv_header(yield);
	BOOST_TEST_REQUIRE(header.get_type() == types::EXECUTE);
	BOOST_TEST(sess.recv_msg(header.get_payload_size(), yield) == cmd);
	sess.send_msg(types::OK, result, yield);
}

void execute_send(const std::string &cmd, const std::string &result, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	auto res = sess.execute(cmd, yield);
	BOOST_TEST_REQUIRE(!res.err);
	BOOST_TEST(res.result == result);
}

BOOST_AUTO_TEST_CASE(execute)
{
	std::string cmd = "quit";
	std::string result = "quit result";
	helper hlp;
	hlp.spawn(execute_recv, cmd, result);
	hlp.spawn(execute_send, cmd, result);
	hlp.run();
}

void hello_recv(const std::string &msg, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	auto header = sess.recv_header(yield);
	BOOST_TEST_REQUIRE(header.get_type() == types::HELLO);
	BOOST_TEST(sess.recv_msg(header.get_payload_size(), yield) == msg);
	sess.send_msg(types::OK, yield);
}

void hello_send(const std::string &msg, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	auto result = sess.hello(msg, yield);
	BOOST_TEST(!result.err);
}

BOOST_AUTO_TEST_CASE(hello)
{
	std::string msg = "hello world";
	helper hlp;
	hlp.spawn(hello_recv, msg);
	hlp.spawn(hello_send, msg);
	hlp.run();
}

void quit_recv(const std::string &msg, helper &hlp, yield_context yield)
{
	auto sess = hlp.accept(yield);
	auto header = sess.recv_header(yield);
	BOOST_TEST_REQUIRE(header.get_type() == types::QUIT);
	BOOST_TEST(sess.recv_msg(header.get_payload_size(), yield) == msg);
	sess.send_msg(types::OK, yield);
}

void quit_send(const std::string &msg, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	auto result = sess.quit(msg, yield);
	BOOST_TEST(!result.err);
}

BOOST_AUTO_TEST_CASE(quit)
{
	std::string msg = "quit";
	helper hlp;
	hlp.spawn(quit_recv, msg);
	hlp.spawn(quit_send, msg);
	hlp.run();
}

void send_file_recv_multiple(unsigned count, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	for (unsigned i = 0; i < count; i++)
	{
		hlp.spawn(send_file_recv, path, file_content);
	}
}

void send_file_send_multiple(unsigned count, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	for (unsigned i = 0; i < count; i++)
		hlp.spawn(send_file_send, path, file_content);
}

BOOST_AUTO_TEST_CASE(send_file_multiple)
{
	helper hlp;
	std::filesystem::path path = R"(C:\some\file\path\file.txt)";
	auto file_content = create_file_content();
	unsigned count = 5;
	hlp.spawn(send_file_recv_multiple, count, path, file_content);
	hlp.spawn(send_file_send_multiple, count, path, file_content);
	hlp.run();
}