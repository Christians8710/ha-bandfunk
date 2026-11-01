#pragma once
#include <stdint.h>

// CC1101 SWRS061I: asynchronous OOK on GDO0, PATABLE[0/1] off/on.
// Checks confirm chip configuration/state, never reception or RF output power.
struct TxCheck {
  uint8_t idleState=0xFF, txState=0xFF, calibration=0xFF;
  uint8_t mismatchAddress=0xFF, expectedValue=0xFF, actualValue=0xFF;
  bool configVerified=false;
  const char* error="Noch kein Sendeversuch";
};

template<class Radio> bool prepareRadioTX(Radio& radio, TxCheck& check) {
  check=TxCheck();
  if (!radio.strobe(0x36) || !radio.waitState(0x01,check.idleState)) {
    check.error="CC1101 erreicht IDLE nicht"; return false;
  }
  if (!radio.write(0x02,0x2E) || !radio.write(0x22,0x11) || !radio.writePA(0,0x34)) {
    check.error="SPI-Schreibfehler"; return false;
  }
  const uint8_t expected[][2] = {
    {0x02,0x2E},{0x08,0x32},{0x0A,0x00},{0x0C,0x00},
    {0x0D,0x10},{0x0E,0xB0},{0x0F,0x71},
    {0x10,0x67},{0x11,0x32},{0x12,0x30},{0x17,0x00},{0x18,0x18},{0x22,0x11}
  };
  for (const auto& reg:expected) {
    uint8_t actual=radio.read(reg[0]);
    if (actual!=reg[1]) {
      check.mismatchAddress=reg[0]; check.expectedValue=reg[1]; check.actualValue=actual;
      check.error="CC1101-Register stimmen nicht"; return false;
    }
  }
  uint8_t off=0xFF,on=0xFF;
  if (!radio.readPA(off,on) || off!=0 || on!=0x34) {
    check.error="OOK-Leistungstabelle stimmt nicht"; return false;
  }
  check.configVerified=true;
  radio.holdLow();
  if (!radio.strobe(0x35) || !radio.waitState(0x13,check.txState)) {
    check.error="CC1101 erreicht TX nicht"; return false;
  }
  check.calibration=radio.read(0x25);
  // TI SWRZ020E: use FSCAL1 rather than unreliable PKTSTATUS PLL bits.
  if (check.calibration==0xFF || check.calibration==0x3F) {
    check.error="PLL-Kalibrierung fehlgeschlagen"; return false;
  }
  check.error=""; return true;
}
