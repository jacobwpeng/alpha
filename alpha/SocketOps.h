/*
 * =============================================================================
 *
 *       Filename:  SocketOps.h
 *        Created:  04/05/15 14:14:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __SOCKET_OPS_H__
#define __SOCKET_OPS_H__

namespace alpha {
namespace SocketOps {
void SetNonBlocking(int fd);
void SetReuseAddress(int fd);
void SetReceiveTimeout(int fd, int microseconds);
int GetAndClearError(int fd);
void DisableReading(int fd);
void DisableWriting(int fd);
void DisableReadingAndWriting(int fd);
}
}
#endif /* ----- #ifndef __SOCKET_OPS_H__  ----- */
