/*
 * =============================================================================
 *
 *       Filename:  BasicAuthorization.h
 *        Created:  11/26/15 16:29:33
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __BASICAUTHORIZATION_H__
#define __BASICAUTHORIZATION_H__

#include <string>

namespace amqp {
struct PlainAuthorization {
  std::string user;
  std::string passwd;
};
}

#endif /* ----- #ifndef __BASICAUTHORIZATION_H__  ----- */
