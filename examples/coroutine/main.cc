/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  03/29/15 13:45:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "player.h"
#include <ctime>
#include <cstdlib>
#include <iostream>
#include "logger.h"

static void LogOutput(alpha::LogLevel level, const char* content, int len) {
    ::fwrite(content, 1, len, stdout);
}

int main(int argc, char* argv[]) {
    (void)argc;
    alpha::Logger::Init(argv[0], LogOutput);
    ::srand(::time(NULL));
    int target = 30;
    NormalPlayer jack(target);
    CleverPlayer lucy(target);

    int current = 0;
    int round = 1;
    do {
        jack.SetCurrentNum(current);
        jack.Resume();
        int choice = jack.GetChosenNum();
        LOG_INFO << "Jack choose " << choice;
        current += choice;
        if (current == target) {
            LOG_INFO << "Jack wins!";
            jack.EndGame();
            lucy.EndGame();
            break;
        }

        lucy.SetCurrentNum(current);
        lucy.Resume();
        choice = lucy.GetChosenNum();
        LOG_INFO << "Lucy choose " << choice;
        current += choice;

        if (current == target) {
            LOG_INFO << "Lucy wins!";
            jack.EndGame();
            lucy.EndGame();
        }
        LOG_INFO << "Round " << round++ << ", current = " << current;
    } while (current != target);
}