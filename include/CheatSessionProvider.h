#pragma once

#include "Title.h"
#include "debugger.h"
#include "dmntcht.h"

#include <functional>

class CheatSessionProvider {
 public:
  static Result doWithCheatSession(std::function<Result()> func);
  static bool dmntPresent();

 protected:
  static Title* s_runningGameTitle;
  static bool s_hasMetadata;
  static DmntCheatProcessMetadata s_metadata;

 private:
  static bool s_sysmodulePresent;
  static Debugger* s_debugger;
};
