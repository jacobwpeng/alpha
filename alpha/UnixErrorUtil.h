/*
 * =============================================================================
 *
 *       Filename:  UnixErrorUtil.h
 *        Created:  01/17/16 19:53:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <string>

namespace alpha {
std::string UnixErrorToString(int errnum);
}
