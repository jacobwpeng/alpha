/*
 * ==============================================================================
 *
 *       Filename:  SimpleHTTPServer.cc
 *        Created:  05/03/15 19:16:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/SimpleHTTPServer.h>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>
#include <alpha/Format.h>
#include <alpha/NetAddress.h>
#include <alpha/EventLoop.h>
#include <alpha/TcpServer.h>
#include <alpha/HTTPMessageCodec.h>

namespace alpha {
SimpleHTTPServer::SimpleHTTPServer(EventLoop* loop) : loop_(loop) {}

SimpleHTTPServer::~SimpleHTTPServer() = default;

bool SimpleHTTPServer::Run(const NetAddress& addr) {
  using namespace std::placeholders;
  server_.reset(new TcpServer(loop_, addr));
  server_->SetOnRead(std::bind(&SimpleHTTPServer::OnMessage, this, _1, _2));
  server_->SetOnNewConnection(
      std::bind(&SimpleHTTPServer::OnConnected, this, _1));
  server_->SetOnClose(std::bind(&SimpleHTTPServer::OnClose, this, _1));
  return server_->Run();
}

void SimpleHTTPServer::DefaultRequestCallback(TcpConnectionPtr conn,
                                              Slice method,
                                              Slice path,
                                              const HTTPHeader& header,
                                              Slice data) {
  LOG_INFO << "New request from " << conn->PeerAddr();
  LOG_INFO << "method = " << method.ToString();
  LOG_INFO << "path = " << path.ToString();
  for (auto& p : header) {
    LOG_INFO << p.first << ": " << p.second;
  }
  if (!data.empty()) {
    LOG_INFO << "data = \n" << HexDump(data);
  }
  conn->Write("{\"result\": \"0\", \"msg\": \"ok\"}");
}

void SimpleHTTPServer::OnMessage(TcpConnectionPtr conn,
                                 TcpConnectionBuffer* buffer) {
  auto codec = conn->GetContext<std::shared_ptr<HTTPMessageCodec>>();
  auto data = buffer->Read();
  // DLOG_INFO << '\n' << alpha::HexDump(data);
  auto data_size = data.size();
  auto status = codec->Process(data);
  int consumed = data_size - data.size();
  assert(consumed >= 0);
  buffer->ConsumeBytes(consumed);
  DLOG_INFO << "codec consume " << consumed << " bytes";
  if (status > 0) {
    // nasty way to handle multipart/form-data
    if (!codec->data_to_peer().empty()) {
      conn->Write(codec->data_to_peer());
      codec->clear_data_to_peer();
    }
  } else if (status == HTTPMessageCodec::Status::kDone) {
    // nasty way to handle multipart/form-data
    if (!codec->data_to_peer().empty()) {
      conn->Write(codec->data_to_peer());
      codec->clear_data_to_peer();
    }
    auto& http_message = codec->Done();
    http_message.SetClientAddress(conn->PeerAddr());
    callback_(conn, http_message);
    if (!conn->closed()) conn->Close();
  } else {
    LOG_WARNING << "Codec error, status = " << status;
    conn->Close();
  }
}

void SimpleHTTPServer::OnConnected(TcpConnectionPtr conn) {
  auto codec = std::make_shared<HTTPMessageCodec>();
  conn->SetContext(codec);
}

void SimpleHTTPServer::OnClose(TcpConnectionPtr conn) { (void)conn; }
}
