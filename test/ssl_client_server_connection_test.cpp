#include <pthread.h>

#include "test_common.h"

#include "linear/ssl_client.h"
#include "linear/ssl_server.h"

#define CLIENT_CERT          "../sample/certs/client.pem"
#define CLIENT_PKEY          "../sample/certs/client.key"
#define SERVER_CERT          "../sample/certs/server.pem"
#define SERVER_PKEY          "../sample/certs/server.key"
#define CIPHER_LIST          "AES128-GCM-SHA256:RC4:HIGH:!MD5:!aNULL:!EDH"
#define CA_CERT              "../sample/certs/ca.pem"

using namespace linear;
using ::testing::_;
using ::testing::InSequence;
using ::testing::DoAll;
using ::testing::WithArg;
using ::testing::WithArgs;
using ::testing::Eq;
using ::testing::ByRef;
using ::testing::Assign;

typedef LinearTest SSLClientServerConnectionTest;

// Refuse
TEST_F(SSLClientServerConnectionTest, ConnectRefuse) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*ch, OnConnectMock(_)).Times(0);
  EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_ECONNREFUSED))).WillOnce(DoAll(Assign(&srv_finished, true), Assign(&cli_finished, true)));

  Error e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

// Timeout
TEST_F(SSLClientServerConnectionTest, ConnectTimeout) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  SSLSocket cs = cl.CreateSocket(TEST_ADDR_4_TIMEOUT, TEST_PORT);

  EXPECT_CALL(*ch, OnConnectMock(_)).Times(0);
  EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_ETIMEDOUT))).WillOnce(DoAll(Assign(&srv_finished, true), Assign(&cli_finished, true)));

  Error e = cs.Connect(1);
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

// Cancel
TEST_F(SSLClientServerConnectionTest, ConnectCancel) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  SSLSocket cs = cl.CreateSocket("127.0.0.2", TEST_PORT);

  EXPECT_CALL(*ch, OnConnectMock(_)).Times(0);
  EXPECT_CALL(*ch, OnDisconnectMock(_, _)).WillOnce(DoAll(Assign(&srv_finished, true), Assign(&cli_finished, true)));

  Error e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  e = cs.Disconnect();
  WAIT_TO_FINISH_CALLBACK();
}

// Disconnect EALREADY
TEST_F(SSLClientServerConnectionTest, DisconnectEalready) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*ch, OnConnectMock(_)).Times(0);
  EXPECT_CALL(*ch, OnDisconnectMock(_, _)).Times(0);

  Error e = cs.Disconnect();
  ASSERT_EQ(LNR_EALREADY, e.Code());
  msleep(WAIT_MSEC);
}

// Connect EALREADY
TEST_F(SSLClientServerConnectionTest, ConnectEalready) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();

  e = cs.Connect();
  ASSERT_EQ(LNR_EALREADY, e.Code());
}

// Connect EINVAL
TEST_F(SSLClientServerConnectionTest, ConnectEinval) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();

  e = sh->s_.Connect();
  ASSERT_EQ(LNR_EINVAL, e.Code());
}

// Connect - Disconnect from Client in front thread
TEST_F(SSLClientServerConnectionTest, DisconnectFromClientFT) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();

  EXPECT_CALL(*sh, OnDisconnectMock(sh->s_, Error(LNR_EOF))).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_OK))).WillOnce(Assign(&cli_finished, true));

  e = cs.Disconnect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

// Connect - Disconnect from Server in front thread
TEST_F(SSLClientServerConnectionTest, DisconnectFromServerFT) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();

  EXPECT_CALL(*sh, OnDisconnectMock(sh->s_, Error(LNR_OK))).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_EOF))).WillOnce(Assign(&cli_finished, true));

  sh->s_.Disconnect();
  WAIT_TO_FINISH_CALLBACK();
}

// Connect - Disconnect from Client in back thread
TEST_F(SSLClientServerConnectionTest, DisconnectFromClientBT) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  {
    InSequence dummy;
    EXPECT_CALL(*sh, OnConnectMock(_));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), Error(LNR_EOF))).WillOnce(Assign(&srv_finished, true));;
  }
  {
    InSequence dummy;
    EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(WithArg<0>(Disconnect()));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_OK))).WillOnce(Assign(&cli_finished, true));;
  }

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

// Connect - Disconnect from Server in back thread
TEST_F(SSLClientServerConnectionTest, DisconnectFromServerBT) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);
  Socket ss;

  {
    InSequence dummy;
    EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(WithArg<0>(Disconnect()));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), Error(LNR_OK))).WillOnce(Assign(&srv_finished, true));
  }
  {
    InSequence dummy;
    EXPECT_CALL(*ch, OnConnectMock(cs));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_EOF))).WillOnce(Assign(&cli_finished, true));;
  }

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

// Reconnect at same socket
TEST_F(SSLClientServerConnectionTest, Reconnect) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  {
    InSequence dummy;
    EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(WithArg<0>(Disconnect()));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), _));
    EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(WithArg<0>(Disconnect()));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), _)).WillOnce(Assign(&srv_finished, true));
  }
  {
    InSequence dummy;
    EXPECT_CALL(*ch, OnConnectMock(cs));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_EOF))).WillOnce(WithArg<0>(Connect()));
    EXPECT_CALL(*ch, OnConnectMock(cs));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_EOF))).WillOnce(Assign(&cli_finished, true));
  }

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}

namespace global {
extern linear::Socket gs_;
}

// Connect - Disconnect delayed Socket Destruct: must not SEGV
TEST_F(SSLClientServerConnectionTest, DelayedSocketDestruct) {
  linear::shared_ptr<DelayedMockHandler> ch = linear::shared_ptr<DelayedMockHandler>(new DelayedMockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  {
    InSequence dummy;
    EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(WithArg<0>(Disconnect()));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), Error(LNR_OK))).WillOnce(Assign(&srv_finished, true));
  }

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());
  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  {
    InSequence dummy;
    EXPECT_CALL(*ch, OnConnectMock(cs));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_EOF))).WillOnce(Assign(&cli_finished, true));
  }

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
  ASSERT_EQ(global::gs_, cs);

  cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
  ASSERT_NE(global::gs_, cs);

  EXPECT_CALL(*sh, OnDisconnectMock(_, _)).Times(::testing::AtLeast(0));
  EXPECT_CALL(*ch, OnDisconnectMock(_, _)).Times(::testing::AtLeast(0));
}

static void* call_from_thread(void* param) {
  global::gs_.Disconnect();
  return NULL;
}

ACTION(DisconnectFromOtherThread_SSL) {
  pthread_t thread;
  ASSERT_EQ(0, pthread_create(&thread, NULL, call_from_thread, NULL));
  pthread_join(thread, NULL);
  linear::SSLSocket ssl = global::gs_.as<linear::SSLSocket>();
  ASSERT_EQ(LNR_ENOTCONN, ssl.SetSockOpt(SOL_SOCKET, SO_KEEPALIVE, NULL, 0).Code());
  ASSERT_EQ(LNR_ENOTCONN, ssl.GetVerifyResult().Code());
  ASSERT_EQ(false, ssl.PresentPeerCertificate());
  ASSERT_THROW(ssl.GetPeerCertificate(), std::runtime_error);
}

// Connect - Disconnect from other thread, and check certificate: must not SEGV
TEST_F(SSLClientServerConnectionTest, OnConnectAndDisconnectFromOtherTherad) {
  linear::shared_ptr<ThreadMockHandler> ch = linear::shared_ptr<ThreadMockHandler>(new ThreadMockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  {
    InSequence dummy;
    EXPECT_CALL(*sh, OnConnectMock(_));
    EXPECT_CALL(*sh, OnDisconnectMock(Eq(ByRef(sh->s_)), Error(LNR_EOF))).WillOnce(Assign(&srv_finished, true));
  }

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());
  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  {
    InSequence dummy;
    EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(WithArg<0>(DisconnectFromOtherThread_SSL()));
    EXPECT_CALL(*ch, OnDisconnectMock(cs, Error(LNR_OK))).WillOnce(Assign(&cli_finished, true));
  }

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
  ASSERT_NE(global::gs_, cs);

  EXPECT_CALL(*sh, OnDisconnectMock(_, _)).Times(::testing::AtLeast(0));
  EXPECT_CALL(*ch, OnDisconnectMock(_, _)).Times(::testing::AtLeast(0));
}

ACTION(VerifySSL) {
  linear::Socket s = arg0;
  linear::SSLSocket ss = s.as<linear::SSLSocket>();
  ASSERT_EQ(linear::LNR_OK, ss.GetVerifyResult().Code());
}
// Verify Server Cert
TEST_F(SSLClientServerConnectionTest, VerifyServerCert) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(Assign(&srv_finished, true));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(DoAll(WithArg<0>(VerifySSL()), Assign(&cli_finished, true)));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}
// Verify Client Cert
TEST_F(SSLClientServerConnectionTest, VerifyClientCert) {
  linear::shared_ptr<MockHandler> ch = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext context(SSLContext::TLSv1_1);
  context.SetCertificate(std::string(CLIENT_CERT));
  context.SetPrivateKey(std::string(CLIENT_PKEY));
  context.SetCAFile(std::string(CA_CERT));
  context.SetCiphers(std::string(CIPHER_LIST));
  context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLClient cl(ch, context);
  linear::shared_ptr<MockHandler> sh = linear::shared_ptr<MockHandler>(new MockHandler());
  SSLContext server_context(SSLContext::TLSv1_1);
  server_context.SetCertificate(std::string(SERVER_CERT));
  server_context.SetPrivateKey(std::string(SERVER_PKEY));
  server_context.SetCAFile(std::string(CA_CERT));
  server_context.SetCiphers(std::string(CIPHER_LIST));
  server_context.SetVerifyMode(SSLContext::VERIFY_PEER);
  SSLServer sv(sh, server_context);

  Error e = sv.Start(TEST_ADDR, TEST_PORT);
  ASSERT_EQ(LNR_OK, e.Code());

  SSLSocket cs = cl.CreateSocket(TEST_ADDR, TEST_PORT);

  EXPECT_CALL(*sh, OnConnectMock(_)).WillOnce(DoAll(WithArg<0>(VerifySSL()), Assign(&srv_finished, true)));
  EXPECT_CALL(*ch, OnConnectMock(cs)).WillOnce(Assign(&cli_finished, true));

  e = cs.Connect();
  ASSERT_EQ(LNR_OK, e.Code());
  WAIT_TO_FINISH_CALLBACK();
}
