/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdMessageDispatcher.cc
 *        Created:  12/21/15 21:59:47
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdMessageDispatcher.h"
#include <alpha/logger.h>

namespace ThronesBattle {
int32_t MessageDispatcher::Dispatch(
    UinType uin, const google::protobuf::Message* m,
    ThronesBattleServerProtocol::ResponseWrapper* response_wrapper) {
  auto it = callbacks_.find(m->GetDescriptor());
  if (it != callbacks_.end()) {
    return it->second->OnMessage(uin, m, response_wrapper);
  } else {
    LOG_WARNING << "Cannot dispatch message, " << m->DebugString();
    return -1;
  }
}
}
