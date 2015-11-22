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
#include "Connection.h"

namespace amqp {
class FSM;

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
  using ConnectedCallback = std::function<void(ConnectionPtr& conn)>;
  ConnectionMgr();
  ~ConnectionMgr();
  void Run() { loop_.Run(); }
  void set_connected_callback(const ConnectedCallback& cb) {
    connected_callback_ = cb;
  }

  template <typename... Args>
  void AddFramePacker(alpha::TcpConnection* conn, Args&&... args);
  void ConnectTo(ConnectionParameters params);
  void CloseConnection(Connection* conn);
  void OpenNewChannel(Connection* conn, ChannelID channel_id);

 private:
  struct ConnectionContext final {
    ConnectionContext(alpha::TcpConnectionPtr& conn);
    TcpConnectionWriter w;
    const CodecEnv* codec_env;
    std::unique_ptr<FSM> fsm;
    FrameReader frame_reader;
    ConnectionPtr conn;
    std::vector<FramePacker> frame_packers_;

    void FlushReply();
  };
  void OnConnectionEstablished(ConnectionPtr& conn);
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

template <typename... Args>
void ConnectionMgr::AddFramePacker(alpha::TcpConnection* conn, Args&&... args) {
  auto it = connection_context_map_.find(conn);
  CHECK(it != connection_context_map_.end());
  it->second->frame_packers_.emplace_back(std::forward<Args>(args)...);
}
}

#endif /* ----- #ifndef __CONNECTIONMGR_H__  ----- */
