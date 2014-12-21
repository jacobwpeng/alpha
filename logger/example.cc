/*
 * ==============================================================================
 *
 *       Filename:  example.cc
 *        Created:  12/21/14 06:19:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <iostream>
#include <chrono>

#include "logger.h"
#include "log_file.h"

static alpha::LogFile file("./example.log");
static size_t bytes = 0;
static size_t records = 0;

void Output(alpha::LogLevel level, const char* content, int len) {
    (void)level;
    bytes += len;
    ++records;
    file.Append(content, len);
}

int main(int argc, char* argv[]) {
    using std::chrono::system_clock;
    alpha::Logger::Init(argv[0], Output);
    std::string message(480, '&');
    system_clock::time_point start = system_clock::now();
    const size_t kLoopTimes = 1 << 20;
    for (size_t i = 0; i < kLoopTimes; ++i) {
        LOG_INFO << message;
    }
    system_clock::time_point end = system_clock::now();
    size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << (static_cast<double>(bytes) / (1024 * ms)) << " MiB/s\n";
    std::cout << (static_cast<double>(records * 1024) / ms) << " Records/s\n";
}
