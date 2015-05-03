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
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include "compiler.h"
#include "time_util.h"
#include "timer_manager.h"

namespace alpha {
    class Poller;
    class Channel;

    class EventLoop {
        public:
            enum ServerStatus {
                kIdle = 0,
                kBusy = 1
            };
            using CronFunctor = std::function<int(uint64_t)>;
            using Functor = std::function<void(void)>;

        public:
            EventLoop();
            ~EventLoop();
            DISABLE_COPY_ASSIGNMENT(EventLoop);

            void Run();
            void Quit();
            void QueueInLoop(const Functor& functor);
            void UpdateChannel(Channel * channel);
            void RemoveChannel(Channel * channel);

            TimerManager::TimerId RunAt(alpha::TimeStamp ts, const Functor& f);
            TimerManager::TimerId RunAfter(uint32_t milliseconds, const Functor& f);
            TimerManager::TimerId RunEvery(uint32_t milliseconds, const Functor& f);
            void RemoveTimer(TimerManager::TimerId);
            bool Expired(TimerManager::TimerId) const;
            bool TrapSignal(int signal, const Functor& cb);

            void set_wait_time(int wait_time) { wait_time_ = wait_time; }
            void set_idle_wait_time(int idle_time) { idle_time_ = idle_time; }
            void set_cron_functor(const CronFunctor & functor) {
                cron_functor_ = functor;
            }

        private:
            std::unique_ptr<Poller> poller_;
            bool quit_;
            uint64_t iteration_;
            int wait_time_;
            int idle_time_;
            CronFunctor cron_functor_;
            std::unique_ptr<TimerManager> timer_manager_;
            std::vector<Functor> queued_functors_;
            std::map<int, Functor> signal_handlers_;
    };
}

#endif   /* ----- #ifndef __EVENT_LOOP_H__  ----- */
