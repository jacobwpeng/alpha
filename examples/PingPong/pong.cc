/*
 * =============================================================================
 *
 *       Filename:  pong.cc
 *        Created:  03/22/17 11:04:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/EventLoop.h>
#include <alpha/TcpServer.h>
#include <alpha/Logger.h>

static const std::string kMsgPong = "PONG";

static void OnRead(alpha::TcpConnectionPtr conn,
                   alpha::TcpConnectionBuffer* buffer) {
  size_t len;
  auto data = buffer->Read(&len);
  (void)data;
  buffer->ConsumeBytes(len);
  conn->Write(kMsgPong.data(), kMsgPong.size());
}

static void OnClose(alpha::TcpConnectionPtr conn) {
  alpha::NetAddress peer;
  if (conn->PeerAddr(&peer)) {
    LOG_INFO << peer << " disconnected";
  }
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);

  alpha::EventLoop loop;

  alpha::TcpServer server(&loop, alpha::NetAddress("127.0.0.1", 12345));
  server.SetOnRead(OnRead);
  server.SetOnClose(OnClose);
  server.Run();

  auto quit = [&loop] { loop.Quit(); };
  loop.TrapSignal(SIGINT, quit);
  loop.TrapSignal(SIGTERM, quit);
  loop.Run();
}
