#include "basic_session.h"

void cnc::common::set_timeout(boost::asio::ip::tcp::socket & socket, unsigned int ms)
{
#ifdef _WIN32
	int timeout = ms;
	setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = ms % 1000;
	setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}
