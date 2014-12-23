/*
 * ==============================================================================
 *       Filename:  compiler.h
 *        Created:  15:07:46 Dec 23, 2014
 *         Author:  jacobwpeng
 *          Email:  jacobwpeng@tencent.com
 *    Description:  compiler optimization tools
 *
 * ==============================================================================
 */

#ifndef  __COMPILER_H__
#define  __COMPILER_H__

#ifndef likely
#define likely(x)       __builtin_expect((x),1)
#endif

#ifndef unlikely
#define unlikely(x)     __builtin_expect((x),0)
#endif

#endif   /* ----- #ifndef __COMPILER_H__----- */
