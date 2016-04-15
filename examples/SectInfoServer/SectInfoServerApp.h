/*
 * =============================================================================
 *
 *       Filename:  SectInfoServerApp.h
 *        Created:  04/15/16 15:08:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/event_loop.h>
#include <alpha/UDPServer.h>
#include <alpha/MMapFile.h>
#include <alpha/experimental/RegionBasedHashMap.h>

struct UserSectInfo {
  uint8_t sect;
  uint8_t rank;
  uint16_t level;
  uint8_t reserved[16];
};

class SectInfoServerApp final {
 public:
  SectInfoServerApp();
  int Init(const char* file);
  int Run();

 private:
  using UserMap = alpha::RegionBasedHashMap<uint32_t, UserSectInfo>;
  void HandleFrontEndMessage(alpha::UDPSocket* sock, alpha::IOBuffer* buffer,
                             size_t buf_len, const alpha::NetAddress& source);
  alpha::EventLoop loop_;
  alpha::UDPServer front_end_server_;
  std::unique_ptr<alpha::MMapFile> user_map_file_;
  std::unique_ptr<UserMap> user_map_;
};
