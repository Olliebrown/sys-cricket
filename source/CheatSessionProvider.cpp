#include "CheatSessionProvider.h"
#include "res_macros.h"

bool CheatSessionProvider::s_hasMetadata = false;
bool CheatSessionProvider::s_sysmodulePresent = false;
Debugger* CheatSessionProvider::s_debugger = nullptr;
Title* CheatSessionProvider::s_runningGameTitle = nullptr;

Result CheatSessionProvider::doWithCheatSession(std::function<Result()> func) {
 if (s_debugger == nullptr) {
    s_debugger = new Debugger();

    if (dmntPresent()) {
      dmntchtInitialize();
      if (!s_debugger->m_dmnt || !dmntchtForceOpenCheatProcess()) {
        s_debugger->attachToProcess();
      }
    } else {
      s_debugger->attachToProcess();
    }

    if (!s_debugger->m_dmnt) {
      s_sysmodulePresent = true;
    }
  }

  if (!s_hasMetadata) {
    RETURN_IF_FAIL(dmntchtGetCheatProcessMetadata(&s_metadata));
    s_hasMetadata = true;
  }

  if (s_runningGameTitle == nullptr) {
    s_runningGameTitle = new Title(s_metadata.title_id);
  }

  auto rc = func();
  return rc;
}

bool CheatSessionProvider::dmntPresent() {
  /* Get all process ids */
  u64 process_ids[0x50];
  s32 num_process_ids;
  svcGetProcessList(&num_process_ids, process_ids, sizeof process_ids);  // need to double check

  /* Look for dmnt or dmntgen2 titleID */
  u64 titeID;
  for (s32 i = 0; i < num_process_ids; ++i) {
    if (R_SUCCEEDED(pminfoGetProgramId(&titeID, process_ids[i]))) {
      if (titeID == 0x010000000000000D) {
          return true;
      }
    }
  }
  
  return false;
}
