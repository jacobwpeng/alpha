/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  12/01/15 11:55:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "logger.h"
#include "event_loop.h"
#include "AsyncTcpClient.h"
#include "AsyncTcpConnectionException.h"

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::EventLoop loop;
  alpha::AsyncTcpClient client(&loop);
  client.RunInCoroutine([&loop](alpha::AsyncTcpClient* client,
                                alpha::AsyncTcpConnectionCoroutine* co) {
    auto conn = client->ConnectTo(alpha::NetAddress("119.29.29.29", 80), co);
    if (conn) {
      DLOG_INFO << "Connected";
      std::string reply;
      try {
        conn->Write("GET / HTTP/1.0\r\n\r\n");
        reply += conn->Read(std::numeric_limits<size_t>::max());
      } catch (alpha::AsyncTcpConnectionException& e) {
        DLOG_WARNING << e.what();
        reply += conn->ReadCached();
      }

      DLOG_INFO << '\n' << reply;
    }
    loop.Quit();
  });
  loop.Run();
}
