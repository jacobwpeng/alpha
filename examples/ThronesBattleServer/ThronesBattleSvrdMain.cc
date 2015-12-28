/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdMain.cc
 *        Created:  12/14/15 16:16:08
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstdio>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/AsyncTcpClient.h>
#include <alpha/AsyncTcpConnection.h>
#include <alpha/AsyncTcpConnectionCoroutine.h>
#include <alpha/AsyncTcpConnectionException.h>
#include "netsvrd_frame.h"
#include "ThronesBattleSvrdApp.h"

int main(int argc, char* argv[]) {
  //  if (argc != 2) {
  //    fprintf(stderr, "Usage: %s ConfFile\n", argv[0]);
  //    return EXIT_FAILURE;
  //  }
  // alpha::NetAddress addr("10.6.224.83", 50000);
  // alpha::Logger::set_logtostderr(true);
  alpha::Logger::Init(argv[0]);
#if 0
  alpha::EventLoop loop;
  alpha::AsyncTcpClient client(&loop);
  client.RunInCoroutine([&loop, addr](alpha::AsyncTcpClient* client,
                                      alpha::AsyncTcpConnectionCoroutine* co) {
    std::shared_ptr<alpha::AsyncTcpConnection> conn;
    auto reconnect = [&conn, client, co, addr] {
      if (conn) {
        conn->Close();
        conn = nullptr;
      }
      while (conn == nullptr) {
        conn = client->ConnectTo(addr, co);
        if (conn == nullptr) {
          LOG_WARNING << "Connect failed, sleep";
          co->YieldWithTimeout(2000);
        }
      }
      DLOG_INFO << "Connected";
    };
    while (1) {
      reconnect();
      auto frame = NetSvrdFrame::CreateUnique(40);
      int times = 0;
      while (1) {
        try {
          conn->Write(frame->data(), frame->size());
          ++times;
          LOG_ERROR << "Writed " << times << " times";
        }
        catch (alpha::AsyncTcpConnectionException& e) {
          LOG_WARNING << "Exception: " << e.what();
          reconnect();
        }
      }
    }
  });
  loop.Run();
#endif
#if 1
  ThronesBattle::ServerApp app;
  int err = app.Init(argv[1]);
  if (err) {
    return err;
  }
  return app.Run();
#endif
}
