#ifndef LINEAR_SERVER_IMPL_H_
#define LINEAR_SERVER_IMPL_H_

#include "handler_delegate.h"

namespace linear {

class ServerImpl : public HandlerDelegate {
 public:
  static const int BACKLOG = 10; // backlog = 10
  enum State {
    STOP,
    START
  };

 public:
  ServerImpl(const linear::weak_ptr<linear::Handler>& handler,
             const linear::EventLoop& loop,
             bool show_ssl_version = false)
    : HandlerDelegate(handler, loop, show_ssl_version), state_(STOP) {}
  virtual ~ServerImpl() {}
  virtual linear::Error Start(const std::string& hostname, int port,
                              linear::EventLoopImpl::ServerEvent* ev) = 0;
  virtual linear::Error Stop() = 0;
  virtual void Release(const linear::shared_ptr<linear::SocketImpl>& socket) {
    Group::LeaveAll(Socket(socket));
    HandlerDelegate::Release(socket);
  }
  virtual void OnAccept(tv_stream_t* srv_stream, tv_stream_t* cli_stream, int status) = 0;

 protected:
  linear::ServerImpl::State state_;
  linear::Addrinfo self_;
  linear::mutex mutex_;
};

}  // namespace linear

#endif  // LINEAR_SERVER_H_
