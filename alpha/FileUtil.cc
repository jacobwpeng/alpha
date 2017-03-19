/*
 * =============================================================================
 *
 *       Filename:  FileUtil.cc
 *        Created:  01/16/16 23:22:01
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/FileUtil.h>
#include <unistd.h>

namespace alpha {
bool DeleteFile(alpha::Slice path) { return unlink(path.data()) == 0; }
}
