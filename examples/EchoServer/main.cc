/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  04/05/15 16:00:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <vector>
#include <cassert>
#include <cstdio>
#include <functional>
#include <alpha/Slice.h>
#include <alpha/Logger.h>
#include <alpha/EventLoop.h>
#include <alpha/TcpConnectionBuffer.h>
#include <alpha/TcpConnection.h>
#include <alpha/TcpServer.h>

class EchoServer {
 public:
  EchoServer(alpha::EventLoop* loop, const alpha::NetAddress& addr)
      : loop_(loop), server_(loop, addr) {}

  bool Run() {
    using namespace std::placeholders;
    server_.SetOnNewConnection(
        std::bind(&EchoServer::OnNewConnection, this, _1));
    server_.SetOnRead(std::bind(&EchoServer::OnRead, this, _1, _2));
    return server_.Run();
  }

 private:
  void OnNewConnection(alpha::TcpConnectionPtr conn) {
    LOG_INFO << "New connection from " << conn->PeerAddr();
    UpdateTimer(conn);
  }

  void OnRead(alpha::TcpConnectionPtr conn,
              alpha::TcpConnectionBuffer* buffer) {
    size_t length;
    const char* data = buffer->Read(&length);
    LOG_INFO << "Receive " << length << " bytes from " << conn->PeerAddr();
    conn->Write(data, length);
    buffer->ConsumeBytes(length);
    UpdateTimer(conn);
  }

  void UpdateTimer(alpha::TcpConnectionPtr& conn) {
    auto timerid = conn->GetContextPtr<alpha::TimerManager::TimerId>();
    if (timerid) {
      loop_->RemoveTimer(*timerid);
    }
    auto timer_id = loop_->RunAfter(
        kDefaultTimeout,
        std::bind(
            &EchoServer::KickOff, this, alpha::TcpConnectionWeakPtr(conn)));
    conn->SetContext(timer_id);
  }

  void KickOff(alpha::TcpConnectionWeakPtr weak_conn) {
    if (alpha::TcpConnectionPtr conn = weak_conn.lock()) {
      LOG_INFO << "Kickoff " << conn->PeerAddr();
      auto ctx = conn->GetContext();
      auto timer_id = boost::any_cast<alpha::TimerManager::TimerId>(ctx);
      loop_->RemoveTimer(timer_id);
      conn->Write("Timeout\n");
      conn->Close();
    }
  }

  const uint32_t kDefaultTimeout = 3000;
  alpha::EventLoop* loop_;
  alpha::TcpServer server_;
};

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);
  alpha::EventLoop loop;
  alpha::NetAddress addr("127.0.0.1", 7890);
  EchoServer echo_server(&loop, addr);
  if (!echo_server.Run()) {
    return EXIT_FAILURE;
  }
  loop.Run();
  return EXIT_SUCCESS;
}
