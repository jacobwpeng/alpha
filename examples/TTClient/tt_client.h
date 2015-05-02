/*
 * =============================================================================
 *
 *       Filename:  tt_client.h
 *        Created:  04/30/15 17:22:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TT_CLIENT_H__
#define  __TT_CLIENT_H__

#include <alpha/slice.h>
#include <alpha/tcp_connection.h>

namespace alpha {
    class NetAddress;
    class Coroutine;
    class EventLoop;
    class TcpClient;
}

namespace tokyotyrant {
    class ProtocolCodec;
    enum Code {
        kSuccess = 0,
        kInvalidOperation = 1,
        kNoHost = 2,
        kRefused = 3,
        kSendError = 4,
        kRecvError = 5,
        kExisting = 6,
        kNoRecord = 7,
        kMiscellaneous = 9999
    };

    class Iterator;
    class Client {
        public:
            using MatchKeysCallback = std::function<void(alpha::Slice)>;
            Client(alpha::EventLoop* loop);
            ~Client();
            void SetCoroutine(alpha::Coroutine* co);
            int Connnect(const alpha::NetAddress& addr);
            bool Connected() const;
            int Put(alpha::Slice key, alpha::Slice value);
            int PutKeep(alpha::Slice key, alpha::Slice value);
            int PutCat(alpha::Slice key, alpha::Slice value);
            int PutNR(alpha::Slice key, alpha::Slice value);
            int Out(alpha::Slice key);
            int Get(alpha::Slice key, std::string* val);
            int Stat(std::string* stat);
            int ValueSize(alpha::Slice key, int32_t* size);
            int RecordNumber(int64_t* rnum);
            std::unique_ptr<Iterator> NewIterator();
            int GetForwardMatchKeys(alpha::Slice key, int max_size, MatchKeysCallback cb);
            //Iterator GetForwardMatchKeys(alpha::Slice key);
            //int GetIterator(alpha::Slice key);

        private:
            enum class ConnectionState {
                kConnected = 1,
                kConnecting = 2,
                kDisconnected = 3
            };
            void OnConnectError(const alpha::NetAddress& addr);
            void OnConnected(alpha::TcpConnectionPtr conn);
            void OnDisconnected(alpha::TcpConnectionPtr conn);
            void OnMessage(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer);
            void OnTimeout();

            void Next(Iterator* it);
            std::unique_ptr<ProtocolCodec> CreateCodec(int magic);
            int Request(ProtocolCodec* codec);
            size_t MaxBytesCanWrite();
            bool Write(const uint8_t* buffer, int size);

            friend class Iterator;
            bool connect_error_;
            alpha::EventLoop* loop_;
            alpha::Coroutine* co_;
            alpha::TcpConnectionPtr conn_;
            ConnectionState state_;
            std::unique_ptr<alpha::NetAddress> addr_;
            std::unique_ptr<alpha::TcpClient> tcp_client_;
    };

    class Iterator {
        public:
            void Next();
            std::string key() const;
            int status() const;

        private:
            Iterator(Client* client);
            friend class Client;
            Client* client_;
            int status_ = kSuccess;
            std::string key_;
    };
}

#endif   /* ----- #ifndef __TT_CLIENT_H__  ----- */
