/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdMessageDispatcher.h
 *        Created:  12/21/15 21:56:52
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <map>
#include <memory>
#include <type_traits>
#include <functional>
#include <google/protobuf/message.h>
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include "ThronesBattleSvrd.pb.h"
#include "ThronesBattleSvrdDef.h"

namespace ThronesBattle {
template <typename T>
struct function_traits;

template <typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
  static const size_t nargs = sizeof...(Args);

  typedef R result_type;

  template <size_t i>
  struct arg {
    typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
  };
};

class MessageCallback {
 public:
  virtual ~MessageCallback() = default;
  virtual int32_t OnMessage(
      UinType uin, const google::protobuf::Message* req,
      ThronesBattleServerProtocol::ResponseWrapper* response_wrapper) = 0;
};

template <typename T, typename U>
class ConcreteMessageCallback : public MessageCallback {
 public:
  static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                "T must derive from google::protobuf::Message");
  static_assert(std::is_base_of<google::protobuf::Message, U>::value,
                "U must derive from google::protobuf::Message");
  using RequestType = T;
  using ResponseType = U;
  using CallbackType = std::function<int(UinType uin, const T* req, U* resp)>;
  ConcreteMessageCallback(CallbackType cb) : cb_(cb) {}
  virtual ~ConcreteMessageCallback() = default;
  virtual int32_t OnMessage(
      UinType uin, const google::protobuf::Message* req,
      ThronesBattleServerProtocol::ResponseWrapper* response_wrapper) {
    auto m = dynamic_cast<const RequestType*>(req);
    ResponseType resp;
    auto rc = cb_(uin, m, &resp);
    DLOG_INFO_IF(rc == 0) << "Response: \n" << resp.DebugString();
    response_wrapper->set_payload_name(resp.GetTypeName());
    auto ok = resp.SerializeToString(response_wrapper->mutable_payload());
    if (!ok) {
      LOG_WARNING << "Failed to serialize response";
      return Error::kServerInternalError;
    }
    return rc;
  }

 private:
  CallbackType cb_;
};

class MessageDispatcher {
 public:
  int32_t Dispatch(
      UinType uin, const google::protobuf::Message* m,
      ThronesBattleServerProtocol::ResponseWrapper* response_wrapper);
  template <typename T, typename U>
  void Register(
      const typename ConcreteMessageCallback<T, U>::CallbackType& cb) {
    callbacks_[std::decay<T>::type::descriptor()] =
        alpha::make_unique<ConcreteMessageCallback<
            typename std::decay<T>::type, typename std::decay<U>::type>>(cb);
  }

 private:
  std::map<const google::protobuf::Descriptor*,
           std::unique_ptr<MessageCallback>> callbacks_;
};
};
