#include <assert.h>

#include <sstream>

#include "linear/log.h"
#include "event_loop.h"
#include "wss_socket_impl.h"

namespace linear {

WSSSocketImpl::WSSSocketImpl(const std::string& host, int port,
                             const WSRequestContext& ws_context, const SSLContext& ssl_context,
                             const HandlerDelegate& delegate)
  : SocketImpl(host, port, delegate, Socket::WSS), request_context_(ws_context), ssl_context_(ssl_context) {
}

WSSSocketImpl::WSSSocketImpl(tv_stream_t* stream,
                             const WSRequestContext& ws_context, const SSLContext& ssl_context,
                             const HandlerDelegate& delegate)
  : SocketImpl(stream, delegate, Socket::WSS), request_context_(ws_context), ssl_context_(ssl_context) {
}

WSSSocketImpl::~WSSSocketImpl() {
}

Error WSSSocketImpl::Connect() {
  tv_wss_t* handle = static_cast<tv_wss_t*>(malloc(sizeof(tv_wss_t)));
  if (handle == NULL) {
    return Error(LNR_ENOMEM);
  }
  int ret = tv_wss_init(EventLoop::GetDefault().GetHandle(), handle, ssl_context_.GetHandle());
  if (ret) {
    free(handle);
    return Error(ret);
  }
  std::string url;
  if (request_context_.path.compare(0, 1, "/") == 0) {
    url = request_context_.path;
  } else {
    url = "/" + request_context_.path;
  }
  if (!request_context_.query.empty()) {
    if (request_context_.query.compare(0, 1, "?") == 0 ) {
      url = url + request_context_.query;
    } else {
      url = url + "?" + request_context_.query;
    }
  }
  buffer_reset(&handle->handshake.request.url.raw);
  if (buffer_append(&handle->handshake.request.url.raw, url.c_str(), url.size())) {
    free(handle);
    return Error(LNR_ENOMEM);
  }
  if (request_context_.authenticate.type == AuthContext::BASIC || authenticate_context_.type == AuthContext::DIGEST) {
    std::string uri;
    if (request_context_.path.compare(0, 1, "/") == 0) {
      uri = request_context_.path;
    } else {
      uri = "/" + request_context_.path;
    }
    std::string authorization = authenticate_context_.CreateAuthorizationHeader(uri,
                                                                                request_context_.authenticate.username,
                                                                                request_context_.authenticate.password);
    if (!authorization.empty()) {
      request_context_.headers["Authorization"] = authorization;
    }
  }
  buffer_kv kv;
  buffer_kv_init(&kv);
  for (std::map<std::string, std::string>::iterator it = request_context_.headers.begin();
       it != request_context_.headers.end(); it++) {
    buffer_kv_reset(&kv);
    if (buffer_append(&kv.key, it->first.c_str(), it->first.size()) ||
        buffer_append(&kv.val, it->second.c_str(), it->second.size()) ||
        buffer_kvs_insert(&handle->handshake.request.headers, &kv)) {
      buffer_kv_fin(&kv);
      free(handle);
      return Error(LNR_ENOMEM);
    }
  }
  buffer_kv_fin(&kv);
  stream_ = reinterpret_cast<tv_stream_t*>(handle);
  data_ = new EventLoop::SocketEventData();
  data_->Register(this);
  stream_->data = data_;
  response_context_.headers.clear(); // clear response context
  std::ostringstream port_str;
  port_str << peer_.port;
  if (!bind_ifname_.empty()) {
    ret = tv_bindtodevice(stream_, bind_ifname_.c_str());
    if (ret != 0) {
      LINEAR_LOG(LOG_ERR, "SO_BINDTODEVICE failed(%d)", ret);
      return Error(ret);
    }
  }
  ret = tv_connect(stream_, peer_.addr.c_str(), port_str.str().c_str(), EventLoop::OnConnect);
  if (ret) {
    assert(false); // never reach now
    delete data_;
    data_ = NULL;
    stream_->data = NULL;
    free(stream_);
    stream_ = NULL;
    return Error(ret);
  }
  return Error(LNR_OK);
}

void WSSSocketImpl::OnConnect(tv_stream_t* stream, int status) {
  tv_wss_t* handle = reinterpret_cast<tv_wss_t*>(stream);

  response_context_.code = handle->handshake.response.code;
  for (const buffer_kv* kv = buffer_kvs_get_first(&handle->handshake.response.headers);
       kv; kv = buffer_kvs_get_next(kv)) {
    response_context_.headers[std::string(kv->key.ptr)] = std::string(kv->val.ptr);
  }
  const buffer* auth_val = buffer_kvs_case_find(&handle->handshake.response.headers, CONST_STRING("www-authenticate"));
  if (auth_val != NULL) {
    int nc = (authenticate_context_.nc > 0xfffd) ? 0 : authenticate_context_.nc; // for 32bit
    authenticate_context_ = AuthenticateContextImpl(std::string(auth_val->ptr, auth_val->len));
    authenticate_context_.nc = nc + 1;
  } else {
    authenticate_context_ = AuthenticateContextImpl();
  }
  SocketImpl::OnConnect(stream, status);
}

bool WSSSocketImpl::CheckRetryAuth() {
  return (response_context_.code == WSHS_UNAUTHORIZED &&
          authenticate_context_.type == AuthContext::DIGEST && authenticate_context_.nc < 2);
}

const linear::WSRequestContext& WSSSocketImpl::GetWSRequestContext() {
  return request_context_;
}

void WSSSocketImpl::SetWSRequestContext(const WSRequestContext& request_context) {
  request_context_ = request_context;
}

const linear::WSResponseContext& WSSSocketImpl::GetWSResponseContext() {
  return response_context_;
}

void WSSSocketImpl::SetWSResponseContext(const WSResponseContext& response_context) {
  response_context_ = response_context;
}

Error WSSSocketImpl::GetVerifyResult() {
  int ret = tv_ssl_get_verify_result(reinterpret_cast<tv_wss_t*>(stream_)->ssl_handle);
  if (ret) {
    return Error(ret, stream_->ssl_err);
  } else {
    return Error(LNR_OK);
  }
}

bool WSSSocketImpl::PresentPeerCertificate() {
  X509* xcert = tv_ssl_get_peer_certificate(reinterpret_cast<tv_wss_t*>(stream_)->ssl_handle);
  if (xcert == NULL) {
    return false;
  }
  X509_free(xcert);
  return true;
}

X509Certificate WSSSocketImpl::GetPeerCertificate() {
  X509* xcert = tv_ssl_get_peer_certificate(reinterpret_cast<tv_wss_t*>(stream_)->ssl_handle);
  if (xcert == NULL) {
    throw std::runtime_error("peer certificate does not exist");
  }
  return X509Certificate(xcert);
}

}  // namespace linear
