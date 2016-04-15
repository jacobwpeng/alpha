/*
 * =============================================================================
 *
 *       Filename:  SectInfoServerApp.cc
 *        Created:  04/15/16 15:39:34
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "SectInfoServerApp.h"
#include <functional>
#include <alpha/logger.h>

SectInfoServerApp::SectInfoServerApp() : front_end_server_(&loop_) {}

int SectInfoServerApp::Init(const char* file) {
  (void)file;
  auto open_flags = alpha::MMapFile::kCreateIfNotExists;
  std::string path = "/tmp/user_map_file";
  size_t size = 1 << 26;
  auto user_map_file = alpha::MMapFile::Open(path, size, open_flags);
  if (user_map_file == nullptr) {
    LOG_ERROR << "Open memory maped user_map_file failed, path: " << path;
    return -1;
  }
  user_map_file_ = std::move(user_map_file);
  const char* op = nullptr;
  char* start = reinterpret_cast<char*>(user_map_file_->start());
  if (user_map_file_->newly_created()) {
    op = "Create";
    user_map_ = UserMap::Create(start, user_map_file_->size());
  } else {
    op = "Restore";
    user_map_ = UserMap::Restore(start, user_map_file_->size());
  }
  if (user_map_ == nullptr) {
    LOG_ERROR << op << " user map failed, file: " << path;
  }
  LOG_INFO << op << " user map from " << path << " succeed!";
  LOG_INFO << "User map capacity: " << user_map_->max_size();
  LOG_INFO << "User map size: " << user_map_->size();
  return 0;
}

void SectInfoServerApp::HandleFrontEndMessage(alpha::UDPSocket* sock,
                                              alpha::IOBuffer* buffer,
                                              size_t buf_len,
                                              const alpha::NetAddress& source) {
}

int SectInfoServerApp::Run() {
  using namespace std::placeholders;
  front_end_server_.SetMessageCallback(std::bind(
      &SectInfoServerApp::HandleFrontEndMessage, this, _1, _2, _3, _4));
  front_end_server_.Run(alpha::NetAddress("0.0.0.0", 40000));
  loop_.Run();
  return 0;
}
