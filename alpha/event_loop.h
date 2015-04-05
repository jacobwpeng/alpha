/*
 * =====================================================================================
 *
 *       Filename:  event_loop.h
 *        Created:  08/22/14 14:34:28
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#ifndef  __EVENT_LOOP_H__
#define  __EVENT_LOOP_H__

#include <stdint.h>
#include <signal.h>
#include <vector>
#include <memory>
#include <functional>

namespace alpha
{
    class Poller;
    class Channel;

    class EventLoop {
        public:
            enum ServerStatus {
                kIdle = 0,
                kBusy = 1
            };
            using PeriodFunctor = std::function<int(uint64_t)>;
            using Functor = std::function<void(void)>;

        public:
            EventLoop();
            ~EventLoop();
            EventLoop(EventLoop&&) = delete;
            EventLoop(const EventLoop&) = delete;

            void Run();
            void Quit();
            void QueueInLoop(const Functor& functors);
            void UpdateChannel(Channel * channel);
            void RemoveChannel(Channel * channel);

            void set_wait_time(int wait_time) { wait_time_ = wait_time; }
            void set_idle_wait_time(int idle_time) { idle_time_ = idle_time; }
            void set_period_functor(const PeriodFunctor & functor) { 
                period_functor_ = functor; 
            }

        private:
            std::unique_ptr<Poller> poller_;
            sig_atomic_t quit_;
            uint64_t iteration_;
            int wait_time_;
            int idle_time_;
            std::vector<Functor> queued_functors_;
            PeriodFunctor period_functor_;
    };
}

#endif   /* ----- #ifndef __EVENT_LOOP_H__  ----- */
