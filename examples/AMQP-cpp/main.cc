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

#include <alpha/logger.h>
#include "Connection.h"
#include "ConnectionMgr.h"

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  amqp::ConnectionMgr mgr;
  mgr.set_connected_callback([](amqp::Connection* conn) {
    DLOG_INFO << "amqp::Connection created";
    conn->Close();
  });
  mgr.ConnectTo(amqp::ConnectionParameters());
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
