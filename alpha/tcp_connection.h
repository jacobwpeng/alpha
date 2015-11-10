/*
 * =============================================================================
 *
 *       Filename:  tcp_connection.h
 *        Created:  04/05/15 11:45:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include "slice.h"
#include <memory>
#include <functional>
#include <boost/any.hpp>
#include "compiler.h"
#include "net_address.h"
#include "tcp_connection_buffer.h"

namespace alpha {
class Channel;
class EventLoop;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TcpConnectionWeakPtr = std::weak_ptr<TcpConnection>;

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
 public:
  enum class State {
    kConnected = 1,
    kDisconnected = 2,
  };

  using Context = boost::any;
  using ReadCallback =
      std::function<void(TcpConnectionPtr, TcpConnectionBuffer* buf)>;
  using CloseCallback = std::function<void(int)>;
  using ConnectErrorCallback = std::function<void(TcpConnectionPtr)>;
  using WriteDoneCallback = std::function<void(TcpConnectionPtr)>;

  TcpConnection(EventLoop* loop, int fd, State state);
  ~TcpConnection();
  DISABLE_COPY_ASSIGNMENT(TcpConnection);

  bool Write(const Slice& data);
  void Close();

  void SetOnRead(const ReadCallback& cb) { read_callback_ = cb; }
  void SetOnClose(const CloseCallback& cb) { close_callback_ = cb; }
  void SetOnConnectError(const ConnectErrorCallback& cb) {
    connect_error_callback_ = cb;
  }
  void SetOnWriteDone(const WriteDoneCallback& cb) {
    write_done_callback_ = cb;
  }

  void SetContext(const Context& ctx) { ctx_ = ctx; }
  bool HasContext() const { return !ctx_.empty(); };
  void ClearContext() { ctx_ = Context(); }
  Context GetContext() const { return ctx_; }
  template <typename T>
  T* GetContext() {
    return boost::any_cast<T>(&ctx_);
  }

  bool closed() const { return state_ == State::kDisconnected; }
  int fd() const { return fd_; }
  EventLoop* loop() const { return loop_; }
  State state() const { return state_; }

  NetAddress LocalAddr() const { return *local_addr_; }
  NetAddress PeerAddr() const { return *peer_addr_; }
  TcpConnectionBuffer* ReadBuffer() { return &read_buffer_; }
  TcpConnectionBuffer* WriteBuffer() { return &write_buffer_; }
  size_t BytesCanBytes() const;
  void SetPeerAddr(const NetAddress& addr);

 private:
  void ReadFromPeer();
  void WriteToPeer();
  void ConnectedToPeer();
  void HandleError();
  void GetAddressInfo();

  void InitConnected();
  void Init();
  void CloseByPeer();

 private:
  EventLoop* loop_;
  const int fd_;
  State state_;
  std::unique_ptr<Channel> channel_;
  std::unique_ptr<NetAddress> local_addr_;
  std::unique_ptr<NetAddress> peer_addr_;
  TcpConnectionBuffer read_buffer_;
  TcpConnectionBuffer write_buffer_;
  ReadCallback read_callback_;
  CloseCallback close_callback_;
  ConnectErrorCallback connect_error_callback_;
  WriteDoneCallback write_done_callback_;
  Context ctx_;
};
}

#endif /* ----- #ifndef __TCP_CONNECTION_H__  ----- */
