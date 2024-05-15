#pragma once

#include "debugger.h"
#include "dmntcht.h"
#include "Title.h"

#include <functional>
#include <string>

class GameReader {
 public:
  GameReader();
  ~GameReader();

  Result RefreshMetadata(bool forceRefresh = false);
  Result GetTitleId(u64 *titleId);
  Result GetTitleInfo(std::string &titleName, std::string &titleAuthor, std::string &titleVersion, u8 &errorCode);
  Result GetAllMetadata(char* &metadataJson);
  Result GetIcon(u8* &iconData, u64 &dataSize);

  Result ReadMemoryDirect(bool heapMemory, u64 offset, void *buffer, size_t size);
  Result ReadMemoryPointer(bool heapMemory, const std::vector<u64>& offsets, void *buffer, size_t size);

 private:
  bool m_hasMetadata = false;
  bool m_sysmodulePresent = false;
  Debugger* m_debugger = nullptr;
  Title* m_runningGameTitle = nullptr;

  DmntCheatProcessMetadata m_metadata;
  Result doWithDmntchtSession(std::function<Result()> func);

  static bool dmntPresent();
};
