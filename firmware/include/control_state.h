#pragma once
#include "crowdsync.h"
#include <stddef.h>
#include <string.h>
struct GroupSetting {
  uint8_t r=255,g=255,b=255,brightness=255,effect=0;
  bool on=false;
};
inline uint8_t normalizedBrightness(uint8_t value) { return value>0 && value<3?3:value; }
inline GroupColor appliedColor(const GroupSetting& s) {
  GroupColor c;
  if (!s.on || !s.brightness) return c;
  // Logical 1..255 maps to approximately 5..100% drive; exactly zero stays off.
  uint16_t drive=13+(uint32_t(s.brightness-1)*242+127)/254;
  c.r=(uint32_t(s.r)*drive+2167)/4335;
  c.g=(uint32_t(s.g)*drive+2167)/4335;
  c.b=(uint32_t(s.b)*drive+2167)/4335;
  // A non-black selected color must not vanish solely through 4-bit rounding.
  if (!(c.r || c.g || c.b) && (s.r || s.g || s.b)) {
    uint8_t largest=s.r>s.g?s.r:s.g; if(s.b>largest)largest=s.b;
    c.r=s.r==largest?1:0;c.g=s.g==largest?1:0;c.b=s.b==largest?1:0;
  }
  c.effect=s.effect; return c;
}
inline bool validAdminPassword(const char* value) {
  if (!value) return false;
  size_t length=strlen(value);
  if (length<8 || length>64) return false;
  for (size_t i=0;i<length;++i) if (uint8_t(value[i])<32 || uint8_t(value[i])>126) return false;
  return true;
}
