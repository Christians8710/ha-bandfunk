#pragma once
#include "crowdsync.h"
#include <stddef.h>
#include <string.h>
struct GroupSetting {
  uint8_t r=255,g=255,b=255,brightness=255,effect=0;
  bool on=false;
};
inline GroupColor appliedColor(const GroupSetting& s) {
  GroupColor c;
  if (!s.on || !s.brightness) return c;
  c.r=(uint32_t(s.r)*s.brightness+2167)/4335;
  c.g=(uint32_t(s.g)*s.brightness+2167)/4335;
  c.b=(uint32_t(s.b)*s.brightness+2167)/4335;
  c.effect=s.effect; return c;
}
inline bool validAdminPassword(const char* value) {
  if (!value) return false;
  size_t length=strlen(value);
  if (length<8 || length>64) return false;
  for (size_t i=0;i<length;++i) if (uint8_t(value[i])<32 || uint8_t(value[i])>126) return false;
  return true;
}
