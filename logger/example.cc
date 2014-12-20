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

#include "logger.h"
#include <iostream>

void Output(alpha::LogLevel level, const char* content, int len) {
    std::cout.write(content, len);
}

int main(int argc, char* argv[]) {
    alpha::Logger::Init(argv[0], Output);
    LOG_DEBUG << "Debug";
    LOG_INFO << "Info";
    LOG_WARNING << "Warning";
    LOG_ERROR << "Error";
    LOG_INFO_IF(argc == 1) << "argc == 1";
    LOG_INFO_IF(argc != 1) << "argc != 1";
}
