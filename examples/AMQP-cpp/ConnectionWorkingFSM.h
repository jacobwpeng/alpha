/*
 * =============================================================================
 *
 *       Filename:  ConnectionWorkingFSM.h
 *        Created:  11/22/15 16:38:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "FSM.h"
#include "Connection.h"

namespace amqp {

class ConnectionWorkingFSM final : public FSM {
 public:
  explicit ConnectionWorkingFSM(const ConnectionPtr& conn);
  virtual Status HandleFrame(FramePtr&& frame);
  virtual bool FlushReply();
  virtual bool WriteInitialRequest();

 private:
  ConnectionPtr& conn_;
};
}
