/*
 * ==============================================================================
 *
 *       Filename:  player.h
 *        Created:  03/29/15 13:35:18
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/Coroutine.h>

class PlayerCoroutine : public alpha::Coroutine {
 public:
  PlayerCoroutine(int target);
  virtual ~PlayerCoroutine();
  void SetCurrentNum(int num);
  int GetChosenNum() const;
  void EndGame();

 protected:
  virtual void ChooseNum() = 0;
  int num_;
  int choice_;
  int target_;

 private:
  virtual void Routine();
  bool ended_;
};

class NormalPlayer final : public PlayerCoroutine {
 public:
  NormalPlayer(int target);

 protected:
  virtual void ChooseNum();
};

class CleverPlayer final : public PlayerCoroutine {
 public:
  CleverPlayer(int target);

 protected:
  virtual void ChooseNum();
};
