#include <assert.h>
#include "crowdsync.h"
#include "provision_guard.h"
#include "radio_tx.h"

struct FakeRadio {
  uint8_t regs[64]={}, paOff=0, paOn=0, state=0x0D;
  bool badIdle=false,badTX=false,badWrite=false,badPA=false,badFrequency=false,txRequested=false,dataLow=false;
  FakeRadio() {
    regs[0x08]=0x32; regs[0x0D]=0x10; regs[0x0E]=0xB0; regs[0x0F]=0x71;
    regs[0x10]=0x67; regs[0x11]=0x32; regs[0x12]=0x30; regs[0x18]=0x18; regs[0x25]=0x1F;
  }
  bool strobe(uint8_t command) {
    if (command==0x36) state=badIdle?0x0D:0x01;
    if (command==0x35) { assert(dataLow); txRequested=true; state=badTX?0x16:0x13; }
    return true;
  }
  bool waitState(uint8_t wanted,uint8_t& observed) { observed=state; return state==wanted; }
  bool write(uint8_t address,uint8_t value) { regs[address]=value; return !badWrite; }
  uint8_t read(uint8_t address) { return badFrequency && address==0x0F?0x70:regs[address]; }
  bool writePA(uint8_t off,uint8_t on) { paOff=off;paOn=on;return true; }
  bool readPA(uint8_t& off,uint8_t& on) { off=paOff;on=badPA?0:paOn;return true; }
  void holdLow() { assert(regs[0x02]==0x2E); dataLow=true; }
};

void testRadioTX() {
  TxCheck check;
  FakeRadio ok; assert(prepareRadioTX(ok,check));
  assert(check.configVerified && check.txState==0x13 && check.calibration==0x1F && !check.error[0]);
  FakeRadio idle; idle.badIdle=true; assert(!prepareRadioTX(idle,check)); assert(!idle.txRequested);
  FakeRadio write; write.badWrite=true; assert(!prepareRadioTX(write,check)); assert(!write.txRequested);
  FakeRadio freq; freq.badFrequency=true; assert(!prepareRadioTX(freq,check)); assert(!freq.txRequested);
  FakeRadio pa; pa.badPA=true; assert(!prepareRadioTX(pa,check)); assert(!pa.txRequested);
  FakeRadio tx; tx.badTX=true; assert(!prepareRadioTX(tx,check)); assert(check.txState==0x16);
  FakeRadio pll; pll.regs[0x25]=0x3F; assert(!prepareRadioTX(pll,check)); assert(check.error[0]);
  FakeRadio spi; spi.regs[0x25]=0xFF; assert(!prepareRadioTX(spi,check));
}
int main() {
  testRadioTX();
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
