/*
 * =============================================================================
 *
 *       Filename:  FrameCodec.h
 *        Created:  10/21/15 14:33:10
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __FRAMECODEC_H__
#define  __FRAMECODEC_H__

#include <functional>
#include <alpha/slice.h>

namespace amqp {

class Frame;
class FrameCodec {
  public:
    using NewFrameCallback = std::function<void(Frame*)>;
    FrameCodec();
    size_t Process(alpha::Slice data);
    void SetNewFrameCallback(const NewFrameCallback& cb);

  private:
    Frame* frame_;
    NewFrameCallback new_frame_callback_;
};
}

#endif   /* ----- #ifndef __FRAMECODEC_H__  ----- */
