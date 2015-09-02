/**
 * @file tcp_server.h
 * TCPServer class definition
 */

#ifndef LINEAR_TCP_SERVER_H_
#define LINEAR_TCP_SERVER_H_

#include "linear/handler.h"
#include "linear/server.h"

namespace linear {

/**
 * @class TCPServer tcp_server.h "linear/tcp_server.h"
 *
 * TCPServer class that extends Server class
 * @includelineno tcp_server_sample.cpp
 */
class LINEAR_EXTERN TCPServer : public Server {
 public:
  /// @cond hidden
  TCPServer() : Server() {}
  ~TCPServer();
  /// @endcond
  /**
   * TCPServer Constructor
   * @param [in] handler application defined behavior.
   */
  explicit TCPServer(const linear::Handler& handler);
};

}  // namespace linear

#endif  // LINEAR_TCP_SERVER_H_
