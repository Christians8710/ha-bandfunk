#pragma once
#include <stddef.h>
#include <string.h>
// A per-boot nonce protects the AP-only form, independently of browser Origin.
inline bool allowProvision(bool portalActive, bool fromAP, const char* expected, const char* submitted) {
  if (!portalActive || !fromAP || !expected || !submitted) return false;
  if (strlen(expected)!=32 || strlen(submitted)!=32) return false;
  unsigned difference=0;
  for (size_t i=0;i<32;++i) difference|=static_cast<unsigned char>(expected[i]^submitted[i]);
  return difference==0;
}
