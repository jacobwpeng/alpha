/*
 * ==============================================================================
 *
 *       Filename:  player.cc
 *        Created:  03/29/15 13:39:12
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "player.h"
#include <cassert>
#include <cstdlib>

PlayerCoroutine::PlayerCoroutine(int target) : target_(target), ended_(false) {}

PlayerCoroutine::~PlayerCoroutine() {}

void PlayerCoroutine::SetCurrentNum(int num) { num_ = num; }

int PlayerCoroutine::GetChosenNum() const { return choice_; }

void PlayerCoroutine::EndGame() { ended_ = true; }

void PlayerCoroutine::Routine() {
  while (!ended_) {
    ChooseNum();
    Yield();
  }
}

NormalPlayer::NormalPlayer(int target) : PlayerCoroutine(target) {}

void NormalPlayer::ChooseNum() {
  int delta = target_ - num_;
  assert(delta > 0);
  if (delta <= 2) {
    choice_ = delta;
  } else {
    int r = rand();
    choice_ = r % 2 + 1;
  }
}

CleverPlayer::CleverPlayer(int target) : PlayerCoroutine(target) {}

void CleverPlayer::ChooseNum() {
  /* 咱比较聪明 */
  int mod = num_ % 3;
  if (mod == 0) {
    /* 没办法，这种情况赢不了 */
    choice_ = rand() % 2 + 1;
  } else {
    choice_ = 3 - mod;
  }
}
