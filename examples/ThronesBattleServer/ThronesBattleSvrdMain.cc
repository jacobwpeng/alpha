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
#include <memory>
#include <alpha/logger.h>
#include <alpha/format.h>
#include <alpha/event_loop.h>
#include <alpha/AsyncTcpClient.h>
#include <alpha/AsyncTcpConnection.h>
#include "netsvrd_frame.h"
#include "fightsvrd.pb.h"
#include "ThronesBattleSvrdTaskBroker.h"
#include "ThronesBattleSvrdApp.h"

std::unique_ptr<NetSvrdFrame> ReadFrame(alpha::AsyncTcpConnection* conn) {
  auto data = conn->Read(NetSvrdFrame::kHeaderSize);
  auto reply_frame_header = reinterpret_cast<const NetSvrdFrame*>(data.data());
  if (reply_frame_header->magic != NetSvrdFrame::kMagic) {
    return nullptr;
  }
  auto payload = conn->Read(reply_frame_header->payload_size);
  auto frame = std::unique_ptr<NetSvrdFrame>(
      new (reply_frame_header->payload_size) NetSvrdFrame);
  memcpy(frame.get(), data.data(), data.size());
  memcpy(frame->payload, payload.data(), payload.size());
  return std::move(frame);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s ConfFile\n", argv[0]);
    return EXIT_FAILURE;
  }
  alpha::Logger::set_logtostderr(true);
  alpha::Logger::Init(argv[0]);
  ThronesBattle::ServerApp app;
  int err = app.Init(argv[1]);
  if (err) {
    return err;
  }
  return app.Run();
#if 0
  alpha::EventLoop loop;
  alpha::AsyncTcpClient client(&loop);
  client.RunInCoroutine([&loop](alpha::AsyncTcpClient* client,
                                alpha::AsyncTcpConnectionCoroutine* co) {

    while (1) {
      std::vector<uint32_t> one_camp_warriors(2000, 627650435);
      std::vector<uint32_t> another_camp_warriors(1000, 2191195);

      auto sz = std::min<size_t>(one_camp_warriors.size(),
                                 another_camp_warriors.size());
      one_camp_warriors.resize(sz);
      another_camp_warriors.resize(sz);
      CHECK(sz != 0);
      size_t total = 0;
      auto cb = [&total](const FightServerProtocol::TaskResult& result) {
        ++total;
        DLOG_INFO << "Total: " << total;
        // WriteFeeds
      };

      ThronesBattle::TaskBroker broker(client, co, one_camp_warriors,
                                       another_camp_warriors, cb);

      broker.Wait();
      break;
    }
    loop.Quit();
  });
  loop.Run();
#endif
}
