/*
 * =============================================================================
 *
 *       Filename:  MethodArgs.h
 *        Created:  10/21/15 16:45:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __METHODARGS_H__
#define __METHODARGS_H__

#include <cstdint>
#include <map>
#include "MethodArgTypes.h"
#include "FieldTable.h"

namespace amqp {

struct MethodStartArgs {
  static const ClassID kClassID = 10;
  static const MethodID kMethodID = 10;
  uint8_t version_major = 0;
  uint8_t version_minor = 0;
  FieldTable server_properties;
  LongString mechanisms;
  LongString locales;
};

struct MethodStartOkArgs {
  FieldTable client_properties;
  ShortString mechanism;
  LongString response;
  ShortString locale;
};

struct MethodTuneArgs {
  static const ClassID kClassID = 10;
  static const MethodID kMethodID = 30;
  uint16_t channel_max = 0;
  uint32_t frame_max = 0;
  uint16_t heartbeat_delay = 0;
};

struct MethodTuneOkArgs {
  uint16_t channel_max;
  uint32_t frame_max;
  uint16_t heartbeat_delay;
};

struct MethodOpenArgs {
  ShortString vhost_path;
  bool capabilities;
  bool insist;
};

struct MethodOpenOkArgs {
  static const ClassID kClassID = 10;
  static const MethodID kMethodID = 41;
};
}

#endif /* ----- #ifndef __METHODARGS_H__  ----- */
