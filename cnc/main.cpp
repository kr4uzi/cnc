#include "../common/client_session.h"
#include "../common/serialize_filesystem.h"
#include <functional>

using boost::asio::ip::tcp;
using boost::asio::yield_context;
using cnc::client::protocol;
using cnc::client::session;
using types = protocol::types;

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
	sess.recv_msg(header.get_payload_size(), yield);

	sess.send_msg(types::OK, yield);
	std::stringstream ss;
	ss << file_content;
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
}

void send_file_send(const std::filesystem::path &path, const std::string &content, helper &hlp, yield_context yield)
{
	auto sess = hlp.connect(yield);
	send_file_send_handler(sess, path, content, hlp, yield);
}

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

void send_file_recv_multiple(unsigned count, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	for (unsigned i = 0; i < count; i++)
		hlp.spawn(send_file_recv, path, file_content);
}

void send_file_send_multiple(unsigned count, const std::filesystem::path &path, const std::string &file_content, helper &hlp, yield_context yield)
{
	for (unsigned i = 0; i < count; i++)
		hlp.spawn(send_file_send, path, file_content);
}

int main()
{
	helper hlp;
	std::filesystem::path path = R"(C:\some\file\path\file.txt)";
	auto file_content = create_file_content();
	unsigned count = 5;
	hlp.spawn(send_file_recv_multiple, count, path, file_content);
	hlp.spawn(send_file_send_multiple, count, path, file_content);
	hlp.run();
}