/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  10/19/15 14:21:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/event_loop.h>
#include <alpha/logger.h>
#include <alpha/tcp_client.h>
#include <alpha/format.h>
#include "Frame.h"
#include "FrameCodec.h"
#include "CodedWriter.h"
#include "CodedOutputStream.h"
#include "MethodPayloadCodec.h"
#include "FieldValue.h"
#include "CodecEnv.h"
#include "ConnectionEstablishFSM.h"

static alpha::TcpConnectionPtr connection;
static const uint8_t kMajorVersion = 9;
static const uint8_t kMinorVersion = 1;
static amqp::FrameCodec codec;
void OnNewFrame(alpha::TcpConnectionPtr& conn, amqp::FramePtr&& frame);

void OnMessage(alpha::TcpConnectionPtr conn,
               alpha::TcpConnectionBuffer* buffer) {
  auto data = buffer->Read();
  DLOG_INFO << "Receive " << data.size() << " bytes from server";
  DLOG_INFO << '\n' << alpha::HexDump(data);
  auto consumed = data.size();
  auto frame = codec.Process(data);
  consumed -= data.size();
  DLOG_INFO << "FrameCodec consumed " << consumed << " bytes";
  if (frame) {
    OnNewFrame(conn, std::move(frame));
  }
  buffer->ConsumeBytes(consumed);
}

void OnConnected(alpha::TcpConnectionPtr conn) {
  using namespace std::placeholders;
  DLOG_INFO << "Connected to " << conn->PeerAddr().FullAddress();
  // connection = conn;
  conn->SetOnRead(std::bind(OnMessage, _1, _2));
  auto w = new amqp::TcpConnectionWriter(conn);
  auto fsm = new amqp::ConnectionEstablishFSM(w, amqp::GetCodecEnv(""));
  conn->SetContext(fsm);
}

void OnConnectError(const alpha::NetAddress& addr) {
  LOG_WARNING << "Connect to " << addr.FullAddress() << " failed";
}

void OnDisconnected(alpha::TcpConnectionPtr conn) {
  DLOG_INFO << "Disconnected from " << conn->PeerAddr().FullAddress();
  // connection.reset();
}

// void OnWriteDone(alpha::TcpConnectionPtr conn) {
//  auto encoder = conn->GetContext();
//  if (encoder) {
//    bool done = encoder->WriteTo(conn);
//    if (done) {
//      conn->ClearContext();
//    }
//  }
//}

void OnWriteDone(alpha::TcpConnectionPtr conn,
                 amqp::ConnectionEstablishFSM* fsm) {
  if (fsm->FlushReply()) {
    conn->SetOnWriteDone(nullptr);
  }
}

void OnNewFrame(alpha::TcpConnectionPtr& conn, amqp::FramePtr&& frame) {
  DLOG_INFO << "Frame type: " << static_cast<int>(frame->type())
            << ", Frame channel: " << frame->channel_id()
            << ", Frame expected payload size: "
            << frame->expected_payload_size()
            << ", Frame paylaod size: " << frame->payload_size();
  auto p = conn->GetContext<amqp::ConnectionEstablishFSM*>();
  CHECK(p && *p);
  auto fsm = *p;
  auto status = fsm->HandleFrame(std::move(frame));
  switch (status) {
    case amqp::ConnectionEstablishFSM::Status::kDone:
      DLOG_INFO << "Connection established";
      break;
    case amqp::ConnectionEstablishFSM::Status::kWaitMoreFrame:
      DLOG_INFO << "FSM needs more frames";
      break;
    case amqp::ConnectionEstablishFSM::Status::kWaitForWrite:
      DLOG_INFO << "FSM is waiting for more space to write reply";
      conn->SetOnWriteDone(std::bind(OnWriteDone, std::placeholders::_1, fsm));
      break;
  }
}

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  // if (argc != 3) {
  //   LOG_INFO << "Usage: " << argv[0] << " [ip] [port]";
  //   return -1;
  // }
  std::string ip = "127.0.0.1";
  int port = 5672;
  alpha::EventLoop loop;
  alpha::TcpClient client(&loop);
  client.SetOnConnected(OnConnected);
  client.SetOnConnectError(OnConnectError);
  client.SetOnClose(OnDisconnected);
  auto addr = alpha::NetAddress(ip, port);
  client.ConnectTo(addr);
  loop.TrapSignal(SIGINT, [&loop] { loop.Quit(); });
  loop.Run();
#if 0
  // Blocking Connection
  auto auth = amqp::PlainAuthorization(user, passwd);
  auto conn = amqp::BlockingConnection(host, port, auth);
  auto channel = conn->NewChannel();
  auto exchange = channel->DeclareExchange(exchange_name, exchange_type,
                                           exchange_properties);
  auto queue = channel->DeclareQueue(queue_name, queue_properties);
  channel->BindQueue(exchange, queue);

  channel->SetQueueMessageCallback(queue, callback);
  while (channel->Wait(timeout))
    ;
#endif
  return 0;
}
