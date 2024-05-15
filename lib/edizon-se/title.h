#pragma once

#include <switch.h>
#include <string>
#include <map>

class Title {
public:
  Title(FsSaveDataInfo& saveInfo);
  Title(u64 applicationId);
  ~Title();

  const std::string& getTitleName() const;
  const std::string& getTitleAuthor() const;
  const std::string& getTitleVersion() const;

  u8* getTitleIcon() const;
  u64 getTitleIconSize() const;
  u64 getTitleID() const;

  u8 getErrorCode() const;
  Result getErrorResult() const;

protected:
  Result initFromApplicationId(u64 applicationId);

private:
  u8* m_titleIcon;
  u64 m_titleIconSize;
  u64 m_titleID;

  std::string m_titleName;
  std::string m_titleAuthor;
  std::string m_titleVersion;

  u8 m_errorCode = 0;
  Result m_errorResult = 0;
};
