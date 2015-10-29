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

#ifndef  __METHODARGS_H__
#define  __METHODARGS_H__

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
}

#endif   /* ----- #ifndef __METHODARGS_H__  ----- */
