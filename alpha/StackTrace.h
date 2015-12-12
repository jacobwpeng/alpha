/*
 * =============================================================================
 *
 *       Filename:  StackTrace.h
 *        Created:  12/12/15 23:21:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

namespace alpha {
int GetStackTrace(void** result, int max_depth, int skip_count);
}
