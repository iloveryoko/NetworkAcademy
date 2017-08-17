#include <stdexcept>
#include "PipeWrap.h"
#include "Util/util.h"
#include "Util/uv_errno.h"
#include "Network/sockutil.h"

using namespace std;
using namespace ZL::Util;
using namespace ZL::Network;

#define checkFD(fd) \
	if (fd == -1) { \
		clearFD(); \
		throw runtime_error((StrPrinter << "create windows pipe failed:" << get_uv_errmsg()).operator<<(endl));\
	}

#define closeFD(fd) \
	if (fd != -1) { \
		close(fd);\
		fd = -1;\
	}

namespace ZL {
namespace Poller {

PipeWrap::PipeWrap(){

#if defined(_WIN32)
	_listenerFd = SockUtil::listen(0, "127.0.0.1");
	checkFD(_listenerFd)
	SockUtil::setNoBlocked(_listenerFd,false);
	auto localPort = SockUtil::get_local_port(_listenerFd);
	_pipe_fd[1] = SockUtil::connect("127.0.0.1", localPort,false);
	checkFD(_pipe_fd[1])
	_pipe_fd[0] = accept(_listenerFd, nullptr, nullptr);
	checkFD(_pipe_fd[0])

#else
	if (pipe(_pipe_fd) == -1) {
		throw runtime_error((StrPrinter << "create posix pipe failed:" << get_uv_errmsg()).operator<<(endl));\
	}
#endif // defined(_WIN32)	
	SockUtil::setNoBlocked(_pipe_fd[0],true);
	SockUtil::setNoBlocked(_pipe_fd[1],false);
}

void PipeWrap::clearFD() {
	closeFD(_pipe_fd[0]);
	closeFD(_pipe_fd[1]);

#if defined(_WIN32)
	closeFD(_listenerFd);
#endif // defined(_WIN32)

}
PipeWrap::~PipeWrap(){
	clearFD();
}

int PipeWrap::write(const void *buf, int n) {
#if defined(_WIN32)
	return send(_pipe_fd[1], (char *)buf, n, 0);
#else
	return ::write(_pipe_fd[1],buf,n);
#endif // defined(_WIN32)
}
int PipeWrap::read(void *buf, int n) {
#if defined(_WIN32)
	return recv(_pipe_fd[0], (char *)buf, n, 0);
#else
	return ::read(_pipe_fd[0], buf, n);
#endif // defined(_WIN32)
}

} /* namespace Poller */
} /* namespace ZL*/
