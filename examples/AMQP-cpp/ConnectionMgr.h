/*
 * =============================================================================
 *
 *       Filename:  ConnectionMgr.h
 *        Created:  11/15/15 17:06:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CONNECTIONMGR_H__
#define __CONNECTIONMGR_H__

#include <map>
#include <functional>
#include <alpha/event_loop.h>
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include <alpha/tcp_client.h>
#include "CodedWriter.h"
#include "CodecEnv.h"
#include "FrameCodec.h"

namespace amqp {
class FSM;
class Connection;

struct ConnectionParameters {
  ConnectionParameters();

  std::string host;
  int port;
  std::string vhost;
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat_delay;
  uint16_t connection_attempts;
  uint16_t socket_timeout;
};

class ConnectionMgr {
 public:
  using ConnectedCallback = std::function<void(Connection*)>;
  ConnectionMgr();
  ~ConnectionMgr();
  void Run() { loop_.Run(); }
  void set_connected_callback(const ConnectedCallback& cb) {
    connected_callback_ = cb;
  }
  void ConnectTo(ConnectionParameters params);
  void CloseConnection(Connection* conn);

 private:
  struct ConnectionContext final {
    ConnectionContext(alpha::TcpConnectionPtr& conn);
    TcpConnectionWriter w;
    const CodecEnv* codec_env;
    std::unique_ptr<FSM> fsm;
    FrameReader frame_reader;
    std::unique_ptr<Connection> conn;
  };
  void OnConnectionEstablished(Connection* conn);
  void OnTcpConnected(alpha::TcpConnectionPtr conn);
  void OnTcpMessage(alpha::TcpConnectionPtr conn,
                    alpha::TcpConnectionBuffer* buffer);
  void OnTcpWriteDone(alpha::TcpConnectionPtr conn);
  void OnTcpClosed(alpha::TcpConnectionPtr conn);
  void OnTcpConnectError(const alpha::NetAddress& addr);

 private:
  alpha::EventLoop loop_;
  alpha::TcpClient tcp_client_;
  ConnectedCallback connected_callback_;
  std::map<alpha::TcpConnection*, std::unique_ptr<ConnectionContext>>
      connection_context_map_;
};
}

#endif /* ----- #ifndef __CONNECTIONMGR_H__  ----- */
