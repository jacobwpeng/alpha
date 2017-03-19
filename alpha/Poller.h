/*
 * =====================================================================================
 *
 *       Filename:  Poller.h
 *        Created:  08/22/14 14:40:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =====================================================================================
 */

#pragma once

#include <map>
#include <vector>
#include <chrono>

#include <alpha/Compiler.h>
#include <alpha/TimeUtil.h>
#include <alpha/Channel.h>

struct epoll_event;

namespace alpha {
class Poller {
 public:
  Poller();
  ~Poller();
  DISABLE_COPY_ASSIGNMENT(Poller);

  TimeStamp Poll(int timeout, ChannelList* active_channels);

  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);

 private:
  typedef std::vector<epoll_event> EventList;
  typedef std::map<int, Channel*> ChannelMap;

  void FillActiveChannels(int nevents, ChannelList* active_channels);

 private:
  const static int kMaxFdCount = 100;
  int epoll_fd_;
  ChannelMap channels_;
  EventList events_;
};
}

