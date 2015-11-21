/*
 * =============================================================================
 *
 *       Filename:  FSM.cc
 *        Created:  11/19/15 20:27:31
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "FSM.h"

namespace amqp {
FSM::FSM(CodedWriterBase* w, const CodecEnv* env) : w_(w), codec_env_(env) {}

void FSM::set_codec_env(const CodecEnv* env) { codec_env_ = env; }
}
