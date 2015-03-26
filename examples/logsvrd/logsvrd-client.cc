/*
 * ==============================================================================
 *
 *       Filename:  logsvrd-client.cc
 *        Created:  12/21/14 16:10:34
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <unistd.h>
#include <iostream>
#include <chrono>
#include "bus/process_bus.h"
#include "logger/logger.h"

static size_t bytes = 0;
static size_t records = 0;

static alpha::ProcessBus* g_bus = nullptr;

void Output(alpha::LogLevel level, const char* content, int len) {
    (void)level;
    bytes += len;
    ++records;
    while(!g_bus->Write(content, len));
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cout << "Usage: " << argv[0]
            << " buspath\n";
        return -1;
    }
    using std::chrono::system_clock;
    alpha::Logger::Init(argv[0], Output);

    const char * filepath = argv[1];
    const size_t kFileSize = 1 << 20;

    std::unique_ptr<alpha::ProcessBus> bus(alpha::ProcessBus::RestoreOrCreate(
                filepath, kFileSize));

    if (!bus) {
        std::cout << "RestoreOrCreate failed\n";
        return -1;
    }

    g_bus = bus.get();

    std::string message(480, '&');
    system_clock::time_point start = system_clock::now();
    const size_t kLoopTimes = 1 << 18;
    for (size_t i = 0; i < kLoopTimes; ++i) {
        LOG_INFO << message;
    }
    system_clock::time_point end = system_clock::now();
    size_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << records << " Records\n";
    std::cout << (static_cast<double>(bytes) / (1024 * ms)) << " MiB/s\n";
    std::cout << (static_cast<double>(records * 1024) / ms) << " Records/s\n";
    return 0;
}
