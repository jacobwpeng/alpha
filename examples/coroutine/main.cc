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

int main() {
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
        std::cout << "Jack choose " << choice << '\n';;
        current += choice;
        if (current == target) {
            std::cout << "Jack wins!\n";
            jack.EndGame();
            lucy.EndGame();
            break;
        }

        lucy.SetCurrentNum(current);
        lucy.Resume();
        choice = lucy.GetChosenNum();
        std::cout << "Lucy choose " << choice << '\n';;
        current += choice;

        if (current == target) {
            std::cout << "Lucy wins!\n";
            jack.EndGame();
            lucy.EndGame();
        }
        std::cout << "Round " << round++ << ", current = " << current << '\n';
    } while (current != target);
}
