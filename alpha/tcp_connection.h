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

#ifndef  __TCP_CONNECTION_H__
#define  __TCP_CONNECTION_H__

#include "slice.h"
#include <memory>
#include <functional>
#include <boost/any.hpp>
#include "net_address.h"
#include "tcp_connection_buffer.h"

namespace alpha {
    class Channel;
    class EventLoop;
    class TcpConnection;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using TcpConnectionWeekPtr = std::weak_ptr<TcpConnection>;

    class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
            public:
                enum class State {
                    kConnecting = 1,
                    kConnected = 2,
                    kDisconnected = 3,
                };

                using Context = boost::any;
                using ConnectedCallback = std::function<void(TcpConnectionPtr)>;
                using ReadCallback = 
                    std::function<void(TcpConnectionPtr, TcpConnectionBuffer* buf)>;
                using CloseCallback = std::function<void(int)>;

                TcpConnection(EventLoop* loop, int fd, State state);
                ~TcpConnection();
                TcpConnection(TcpConnection&&) = delete;
                TcpConnection(const TcpConnection&) = delete;

                void Write(const Slice& data);
                void Close();

                void SetOnConnected(const ConnectedCallback& cb) {
                    connected_callback_ = cb;
                }
                void SetOnRead(const ReadCallback& cb) {
                    read_callback_ = cb;
                }
                void SetOnClose(const CloseCallback& cb) {
                    close_callback_ = cb;
                }

                void SetContext( const Context & ctx ) { ctx_ = ctx; }
                void ClearContext() { ctx_ = Context(); }
                Context GetContext() const { return ctx_; }

                int fd() const { return fd_; }
                EventLoop* loop() const { return loop_; }
                State state() const { return state_; }

                NetAddress LocalAddr() const {
                    return *local_addr_;
                }
                NetAddress PeerAddr() const {
                    return *peer_addr_;
                }

            private:
                void ReadFromPeer();
                void WriteToPeer();
                void ConnectedToPeer();
                void HandleError();
                void GetAddressInfo();

                void InitConnected();
                void InitConnecting();

            private:
                EventLoop* loop_;
                const int fd_;
                State state_;
                std::unique_ptr<Channel> channel_;
                std::unique_ptr<NetAddress> local_addr_;
                std::unique_ptr<NetAddress> peer_addr_;
                TcpConnectionBuffer read_buffer_;
                TcpConnectionBuffer write_buffer_;
                ConnectedCallback connected_callback_;
                ReadCallback read_callback_;
                CloseCallback close_callback_;
                Context ctx_;
    };
}

#endif   /* ----- #ifndef __TCP_CONNECTION_H__  ----- */
