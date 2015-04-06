/*
 * ==============================================================================
 *
 *       Filename:  time_util.h
 *        Created:  03/29/15 17:51:43
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */


#ifndef  __TIME_UTIL_H__
#define  __TIME_UTIL_H__

#include <cstdint>

namespace alpha {
    using TimeStamp = uint64_t;
    static const uint32_t kMilliSecondsPerSecond = 1000;
    static const uint32_t kSecondsPerMinutes = 60;
    static const uint32_t kMinutesPerHour = 60;
    static const uint32_t kHoursPerDay = 24;
    static const uint32_t kSecondsPerHour = kSecondsPerMinutes * kMinutesPerHour;
    static const uint32_t kSecondsPerDay = kSecondsPerHour * kHoursPerDay;

    TimeStamp Now();
    TimeStamp NowInSeconds();

    bool InSameHour(TimeStamp lhs, TimeStamp rhs);
}

#endif   /* ----- #ifndef __TIME_UTIL_H__  ----- */
