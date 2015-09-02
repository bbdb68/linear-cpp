/**
 * @file ws_server.h
 * WSServer class definition
 */

#ifndef LINEAR_WS_SERVER_H_
#define LINEAR_WS_SERVER_H_

#include "linear/handler.h"
#include "linear/server.h"
#include "linear/ws_socket.h"

namespace linear {

/**
 * @class WSServer ws_server.h "linear/ws_server.h"
 *
 * WSServer class that extends Server class
 * @includelineno ws_server_sample.cpp
 */
class LINEAR_EXTERN WSServer : public Server {
 public:
  /// @cond hidden
  WSServer() : Server() {}
  ~WSServer();
  /// @endcond
  /**
   * WSServer Constructor
   * @param [in] handler application defined behavior.
   * @param [in] auth_type authentication type
   * @param [in] realm authentication realm
   * @see linear::AuthContext
   */
  WSServer(const linear::Handler& handler,
           linear::AuthContext::Type auth_type = linear::AuthContext::UNUSED,
           const std::string& realm = "Authorization Required");
};

}  // namespace linear

#endif  // LINEAR_WS_SERVER_H_
