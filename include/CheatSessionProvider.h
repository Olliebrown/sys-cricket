#pragma once

#include "debugger.h"
#include "dmntcht.h"
#include "Title.h"

#include <functional>

class CheatSessionProvider {
public:
  static Result doWithCheatSession(std::function<Result()> func);
  static bool dmntPresent();

protected:
  static Title* s_runningGameTitle;
  static DmntCheatProcessMetadata s_metadata;

private:
  static bool s_hasMetadata;
  static bool s_sysmodulePresent;
  static Debugger* s_debugger;
};
