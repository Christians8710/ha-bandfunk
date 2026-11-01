#pragma once
#include <stdint.h>
#include <stddef.h>
// Protocol facts independently implemented from James Wang's MIT-licensed
// CrowdSync_RF, commit b800899687224011657e4ac62caaf45d54892f6d.
struct GroupColor { uint8_t r=0,g=0,b=0,effect=0; };
inline void buildPacket(const GroupColor groups[10], uint8_t packet[22]) {
  uint8_t checksum=0;
  for (size_t i=0;i<10;++i) {
    packet[2*i]=((groups[i].effect&15)<<4)|(groups[i].r&15);
    packet[2*i+1]=((groups[i].g&15)<<4)|(groups[i].b&15);
    checksum^=packet[2*i]; checksum^=packet[2*i+1];
  }
  packet[20]=checksum; packet[21]=0;
}
inline size_t buildWave(const uint8_t packet[22], uint16_t wave[358]) {
  size_t n=0; wave[n++]=120; wave[n++]=600; wave[n++]=120; wave[n++]=1600;
  for (size_t i=0;i<22;++i) for (int b=7;b>=0;--b) {
    bool bit=packet[i]&(1<<b); wave[n++]=bit?320:120; wave[n++]=bit?120:320;
  }
  wave[n++]=120; wave[n++]=2400; return n;
}
