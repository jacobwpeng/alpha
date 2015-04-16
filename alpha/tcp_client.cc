/*
 * =============================================================================
 *
 *       Filename:  tcp_client.cc
 *        Created:  04/06/15 15:52:59
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tcp_client.h"

#include <cassert>

#include "logger.h"
#include "event_loop.h"
#include "net_address.h"
#include "tcp_connector.h"

namespace alpha {
    TcpClient::TcpClient(EventLoop* loop)
        :loop_(loop) {
        assert (loop);
        connector_.reset (new TcpConnector(loop));

        using namespace std::placeholders;
        connector_->SetOnConnect(std::bind(&TcpClient::OnConnect, this, _1, _2, _3));
        connector_->SetOnError(std::bind(&TcpClient::OnConnectError, this, _1));
    }
    
    TcpClient::~TcpClient() {
        for (auto & p : connections_) {
            ::close(p.first);
        }
    }

    void TcpClient::ConnectTo(const NetAddress& addr) {
        loop_->QueueInLoop(std::bind(&TcpConnector::ConnectTo, connector_.get(), addr));
    }

    void TcpClient::OnConnect(int fd, bool connected, const NetAddress& peer_addr) {
        assert (connections_.find(fd) == connections_.end());
        TcpConnection::State state = connected ? TcpConnection::State::kConnected
            : TcpConnection::State::kConnecting;
        TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, fd, state);
        conn->SetPeerAddr(peer_addr);
        if (connected) {
            OnConnected(conn);
        } else {
            using namespace std::placeholders;
            conn->SetOnConnected(std::bind(&TcpClient::OnConnected, this, _1));
        }
        connections_.emplace(fd, conn);
    }

    void TcpClient::OnConnected(TcpConnectionPtr conn) {
        using namespace std::placeholders;
        connected_callback_(conn);
        conn->SetOnRead(read_callback_);
        conn->SetOnClose(std::bind(&TcpClient::OnClose, this, _1));
    }

    void TcpClient::OnConnectError(const NetAddress& addr) {
        LOG_INFO << "Connect to " << addr << " failed";
        if (error_callback_) {
            error_callback_(addr);
        }
    }

    void TcpClient::OnClose(int fd) {
        auto it = connections_.find(fd);
        assert (it != connections_.end());
        TcpConnectionPtr conn = it->second;
        connections_.erase(it);

        DLOG_INFO << "Connection to " << it->second->PeerAddr() << " is closed";
        if (close_callback_) {
            close_callback_(conn);
        }
    }
}
