#include <assert.h>
#include "crowdsync.h"
#include "provision_guard.h"
int main() {
  const char* nonce="0123456789abcdef0123456789abcdef";
  assert(allowProvision(true,true,nonce,nonce));
  assert(!allowProvision(false,true,nonce,nonce));
  assert(!allowProvision(true,false,nonce,nonce));
  assert(!allowProvision(true,true,nonce,""));
  assert(!allowProvision(true,true,nonce,"1123456789abcdef0123456789abcdef"));
  assert(!allowProvision(true,true,"",""));
  assert(!allowProvision(true,true,nonce,nullptr));
  GroupColor groups[10]; uint8_t packet[22]; uint16_t wave[358];
  groups[5].r=15; buildPacket(groups,packet);
  for (int i=0;i<22;++i) assert(packet[i]==((i==10||i==20)?15:0));
  assert(buildWave(packet,wave)==358);
  unsigned sum=0; for (auto t:wave) sum+=t; assert(sum==82400);
  assert(wave[4]==120 && wave[5]==320);
  groups[0].r=15; groups[0].g=15; groups[0].b=15; groups[0].effect=4;
  buildPacket(groups,packet); assert(packet[0]==79 && packet[1]==255);
  assert(packet[20]==(79^255^15)); assert(packet[21]==0);
  groups[0].effect=15; buildPacket(groups,packet); assert(packet[0]==255);
}
