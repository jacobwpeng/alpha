/*
 * ==============================================================================
 *
 *       Filename:  logsvrd.cc
 *        Created:  12/21/14 15:56:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include <unistd.h>
#include <iostream>
#include "bus/process_bus.h"
#include "logger/log_file.h"

int main() {
    const char * filepath = "/tmp/example.bus";
    const size_t kFileSize = 1 << 25;

    FILE * fp = ::fopen("/tmp/logsvrd.log", "a");
    std::unique_ptr<alpha::ProcessBus> bus(alpha::ProcessBus::ConnectTo(
                filepath, kFileSize));

    const useconds_t kNormalInterval = 20; 
    const useconds_t kIdleInterval = 100;
    int idle = 0;
    bool flushed = true;
    int records = 0;
    while (1) {
        int len;
        char * content = bus->Read(&len);
        if (content == nullptr) {
            ++idle;
            if (idle > 100) {
                if (not flushed) {
                    ::fflush(fp);
                    std::cout << records << '\n';
                    flushed = true;
                }
                ::usleep(kIdleInterval);
            } else {
                ::usleep(kNormalInterval);
            }
        } else {
            idle = 0;
            ++records;
            flushed = false;
            ::fwrite(content, sizeof(char), len, fp);
        }
    }
    ::fclose(fp);
    return 0;
}
