/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnectionCoroutine.h
 *        Created:  12/01/15 11:11:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "AsyncTcpConnection.h"

#ifndef __ASYNCTCPCONNECTIONCOROUTINE_H__
#define __ASYNCTCPCONNECTIONCOROUTINE_H__

#include "coroutine.h"

namespace alpha {
class AsyncTcpClient;
class AsyncTcpConnectionCoroutine final : public Coroutine {
 public:
  using CoroutineFunc =
      std::function<void(AsyncTcpClient*, AsyncTcpConnectionCoroutine*)>;
  AsyncTcpConnectionCoroutine(AsyncTcpClient* owner, const CoroutineFunc& func);
  virtual void Routine() override;

 private:
  AsyncTcpClient* owner_;
  CoroutineFunc func_;
};
}

#endif /* ----- #ifndef __ASYNCTCPCONNECTIONCOROUTINE_H__  ----- */
