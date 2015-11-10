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

#ifndef __FRAMECODEC_H__
#define __FRAMECODEC_H__

#include <functional>
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include "Frame.h"

namespace amqp {

class Frame;
class FrameCodec {
 public:
  FramePtr Process(alpha::Slice& data);

 private:
  FramePtr frame_;
};
}

#endif /* ----- #ifndef __FRAMECODEC_H__  ----- */
