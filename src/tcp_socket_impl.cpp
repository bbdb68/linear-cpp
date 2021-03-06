#include <sstream>

#include "tcp_socket_impl.h"

namespace linear {

TCPSocketImpl::TCPSocketImpl(const std::string& host, int port,
                             const shared_ptr<EventLoopImpl>& loop,
                             const weak_ptr<HandlerDelegate>& delegate)
  : SocketImpl(host, port, loop, delegate, Socket::TCP) {
}

TCPSocketImpl::TCPSocketImpl(tv_stream_t* stream,
                             const shared_ptr<EventLoopImpl>& loop,
                             const weak_ptr<HandlerDelegate>& delegate)
  : SocketImpl(stream, loop, delegate, Socket::TCP) {
}

TCPSocketImpl::~TCPSocketImpl() {
}

Error TCPSocketImpl::Connect() {
  stream_ = static_cast<tv_stream_t*>(malloc(sizeof(tv_tcp_t)));
  if (stream_ == NULL) {
    return Error(LNR_ENOMEM);
  }
  int ret = tv_tcp_init(loop_->GetHandle(), reinterpret_cast<tv_tcp_t*>(stream_));
  if (ret) {
    free(stream_);
    return Error(ret);
  }
  if (!bind_ifname_.empty()) {
    ret = tv_bindtodevice(stream_, bind_ifname_.c_str());
    if (ret) {
      free(stream_);
      return Error(ret);
    }
  }
  stream_->data = ev_;
  std::ostringstream port_str;
  port_str << peer_.port;
  ret = tv_connect(stream_, peer_.addr.c_str(), port_str.str().c_str(), EventLoopImpl::OnConnect);
  if (ret) {
    assert(false); // never reach now
    free(stream_);
    return Error(ret);
  }
  return Error(LNR_OK);
}

}  // namespace linear
