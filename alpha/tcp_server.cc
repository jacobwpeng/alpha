/*
 * =============================================================================
 *
 *       Filename:  tcp_server.cc
 *        Created:  04/05/15 15:39:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tcp_server.h"
#include <cassert>
#include "logger.h"
#include "channel.h"
#include "net_address.h"
#include "event_loop.h"
#include "tcp_acceptor.h"

namespace alpha {
    TcpServer::TcpServer(EventLoop* loop, const NetAddress& addr)
        :loop_(loop), listen_addr_(addr) {
            assert (loop_);
    }

    TcpServer::~TcpServer() = default;

    bool TcpServer::Run() {
        using namespace std::placeholders;
        acceptor_.reset (new TcpAcceptor(loop_));
        acceptor_->SetOnNewConnection(std::bind(&TcpServer::OnNewConnection, this, _1));
        if (!acceptor_->Bind(listen_addr_)) {
            return false;
        } else {
            loop_->QueueInLoop(std::bind(&TcpAcceptor::Listen, acceptor_.get()));
            return true;
        }
    }

    void TcpServer::OnNewConnection(int fd) {
        using namespace std::placeholders;
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, fd, 
                TcpConnection::State::kConnected);
        conn->SetOnRead(read_callback_);
        conn->SetOnClose(std::bind(&TcpServer::OnConnectionClose, this, _1));
        assert (connections_.find(fd) == connections_.end());

        connections_.emplace(fd, conn);
        DLOG_INFO << "New Connection, client addr = " << conn->PeerAddr();
        if (new_connection_callback_) {
            new_connection_callback_(conn);
        }
    }

    void TcpServer::OnConnectionClose(int fd) {
        auto it = connections_.find(fd);
        assert (it != connections_.end());
        TcpConnectionPtr conn = it->second;
        connections_.erase(it);

        DLOG_INFO << "Remove connection, peer_addr = " << conn->PeerAddr();
        if (close_callback_) {
            close_callback_(conn);
        }
    }
}
