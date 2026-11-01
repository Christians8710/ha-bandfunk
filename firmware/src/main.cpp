#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <SPI.h>
#include <Updater.h>
#include <BearSSLHelpers.h>
extern "C" {
#include <user_interface.h>
}
#include "ota_key.h"
#include "protocol.h"
#include "crowdsync.h"

// GPIO15/D8 stays completely unused: LOW is required at reset.
constexpr uint8_t CS = D1, DATA = D2, BUTTON = D0;
constexpr uint32_t OTA_WINDOW = 300000, PORTAL_WINDOW = 600000;
ESP8266WebServer server(80);
BearSSL::PublicKey otaKey(OTA_PUBLIC_KEY);
BearSSL::HashSHA256 otaHash;
BearSSL::SigningVerifier otaVerify(&otaKey);
UnknownOOK decoder;
String token, ssid, password, apPassword, deviceId, hostname;
bool portal = false, safeMode = false, radioOK = false;
bool uploadAllowed = false, uploadOK = false, uploadFailed = false;
uint32_t otaUntil = 0, buttonSince = 0, rebootAt = 0;
bool buttonLatched = false, bootCleared = false;
struct BootState { uint32_t magic, count, check; } boot;
volatile uint32_t edgeCount = 0, dropped = 0, lastEdge = 0;
volatile uint16_t head = 0, tail = 0;
volatile uint32_t durations[256];
volatile uint8_t levels[256];
GroupColor groups[10];
uint32_t txUntil=0, lastTx=0, txPackets=0;
volatile bool txRunning=false, txDone=false;
volatile uint16_t waveIndex=0;
uint16_t wave[358];
bool txEnabled() { return txUntil && int32_t(txUntil-millis())>0; }
void IRAM_ATTR txTimer() {
  uint16_t i=++waveIndex;
  if (i>=358) { digitalWrite(DATA,LOW); txRunning=false; txDone=true; return; }
  digitalWrite(DATA,(i&1)?LOW:HIGH);
  timer1_write(uint32_t(wave[i])*5);
}

String randomHex(size_t bytes) {
  String result; result.reserve(bytes * 2);
  for (size_t i = 0; i < bytes; ++i) { char h[3]; snprintf(h, 3, "%02x", uint8_t(os_random())); result += h; }
  return result;
}
bool armed() { return otaUntil && int32_t(otaUntil - millis()) > 0; }
bool authorized() { return server.header("Authorization") == "Bearer " + token; }
bool requireToken() {
  if (authorized()) return true;
  server.send(401, "text/plain; charset=utf-8", "API-Schlüssel erforderlich"); return false;
}
bool requireAdmin() {
  if (server.authenticate("admin", token.c_str())) return true;
  server.requestAuthentication(DIGEST_AUTH, "BandFunk", "Anmeldung erforderlich"); return false;
}
void saveBoot() {
  boot.check = boot.magic ^ boot.count ^ 0xB4F06A11;
  // Offset 64: do not overwrite eboot OTA command at RTC offset zero.
  ESP.rtcUserMemoryWrite(64, reinterpret_cast<uint32_t*>(&boot), sizeof(boot));
}
bool saveConfig() {
  StaticJsonDocument<512> doc;
  doc["token"] = token; doc["ssid"] = ssid; doc["password"] = password;
  File f = LittleFS.open("/config.new", "w");
  if (!f) return false;
  bool ok = serializeJson(doc, f) > 0; f.flush(); f.close();
  return ok && LittleFS.rename("/config.new", "/config.json");
}
bool selectRadio() {
  digitalWrite(CS, LOW);
  uint32_t start = micros();
  while (digitalRead(MISO)) { if (micros() - start > 3000) { digitalWrite(CS,HIGH); return false; } yield(); }
  return true;
}
bool regWrite(uint8_t address, uint8_t value) {
  if (!selectRadio()) return false;
  SPI.transfer(address); SPI.transfer(value); digitalWrite(CS, HIGH); return true;
}
uint8_t regRead(uint8_t address) {
  if (!selectRadio()) return 0xFF;
  SPI.transfer(address | 0xC0); uint8_t v = SPI.transfer(0); digitalWrite(CS,HIGH); return v;
}
bool strobe(uint8_t command) {
  if (!selectRadio()) return false;
  SPI.transfer(command); digitalWrite(CS,HIGH); return true;
}
void IRAM_ATTR onEdge() {
  uint32_t now = micros(); uint32_t dt = now - lastEdge; lastEdge = now;
  ++edgeCount;
  uint16_t next = (head + 1) & 255;
  if (next == tail) { ++dropped; return; }
  durations[head] = dt; levels[head] = !digitalRead(DATA); head = next;
}
bool initRadio() {
  pinMode(CS, OUTPUT); digitalWrite(CS,HIGH); pinMode(DATA,INPUT);
  SPI.begin(); SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(CS,LOW); delayMicroseconds(10); digitalWrite(CS,HIGH); delayMicroseconds(40);
  if (!strobe(0x30)) return false; // SRES
  delay(2);
  uint8_t version = regRead(0x31), part = regRead(0x30);
  if (part != 0 || version == 0 || version == 0xFF) return false;
  // CC1101 26 MHz reference: 433.92 MHz, async OOK, RX only.
  const uint8_t config[][2] = {
    {0x00,0x2E},{0x02,0x0D},{0x03,0x47},{0x06,0xFF},{0x07,0x04},{0x08,0x32},
    {0x0B,0x06},{0x0D,0x10},{0x0E,0xB0},{0x0F,0x71},
    {0x10,0x67},{0x11,0x32},{0x12,0x30},{0x13,0x22},{0x14,0xF8},
    {0x15,0x00},{0x17,0x00},{0x18,0x18},{0x19,0x16},{0x1B,0x43},{0x1C,0x40},{0x1D,0x91},
    {0x21,0x56},{0x22,0x10},{0x23,0xE9},{0x24,0x2A},{0x25,0x00},{0x26,0x1F},
    {0x2C,0x81},{0x2D,0x35},{0x2E,0x09}};
  for (const auto& r : config) if (!regWrite(r[0],r[1])) return false;
  if (!strobe(0x34)) return false; // SRX; never STX
  attachInterrupt(digitalPinToInterrupt(DATA),onEdge,CHANGE); return true;
}
void finishTX() {
  timer1_disable(); strobe(0x36); pinMode(DATA,INPUT); regWrite(0x02,0x0D);
  strobe(0x34); attachInterrupt(digitalPinToInterrupt(DATA),onEdge,CHANGE); txDone=false;
}
void startTX() {
  if (!radioOK || safeMode || portal || armed() || txRunning || txDone) return;
  uint8_t packet[22]; buildPacket(groups,packet); buildWave(packet,wave);
  detachInterrupt(digitalPinToInterrupt(DATA));
  strobe(0x36); regWrite(0x02,0x2E); regWrite(0x22,0x11);
  if (!selectRadio()) { radioOK=false; txUntil=0; return; }
  SPI.transfer(0x7E); SPI.transfer(0); SPI.transfer(0x34); digitalWrite(CS,HIGH);
  pinMode(DATA,OUTPUT); digitalWrite(DATA,LOW);
  if (!strobe(0x35)) { pinMode(DATA,INPUT); radioOK=false; txUntil=0; return; }
  delayMicroseconds(800);
  waveIndex=0; txRunning=true; digitalWrite(DATA,HIGH);
  timer1_enable(TIM_DIV16,TIM_EDGE,TIM_SINGLE); timer1_write(uint32_t(wave[0])*5);
  ++txPackets; lastTx=millis();
}
void sendStatus() {
  if (!requireToken()) return;
  StaticJsonDocument<2048> doc;
  doc["api"]=1; doc["id"]=deviceId; doc["version"]=FW_VERSION;
  doc["protocol"]=decoder.name(); doc["rssi"]=WiFi.RSSI();
  doc["uptime"]=millis()/1000; doc["free_heap"]=ESP.getFreeHeap();
  doc["radio_ok"]=radioOK; doc["safe_mode"]=safeMode; doc["ota_armed"]=armed();
  doc["edges"]=edgeCount; doc["dropped"]=dropped;
  doc["protocol"]="crowdsync_rf_experimentell";
  doc["tx_enabled"]=txEnabled(); doc["tx_packets"]=txPackets;
  JsonArray list=doc.createNestedArray("groups");
  for (const auto& g:groups) { JsonArray a=list.createNestedArray(); a.add(g.r); a.add(g.g); a.add(g.b); a.add(g.effect); }
  String body; serializeJson(doc,body); server.send(200,"application/json",body);
}
const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>BandFunk HA</title>
<h1>BandFunk HA 0.1.0</h1><p>Unabhängiges Forschungsprojekt. Experimenteller CrowdSync-RGB-Treiber nach öffentlicher Protokollreferenz. Deine Platinenvariante ist noch nicht hardwaregeprüft. Aktivierung über Home Assistant für jeweils zwei Minuten.</p>
<p>OTA: Taster D0–GND drei Sekunden halten. Dann ist die Installation fünf Minuten freigegeben. Nur signierte .bin.signed-Dateien werden angenommen.</p>
<form method="post" action="/update" enctype="multipart/form-data"><label>Signierte Firmware <input type="file" name="firmware" required></label><button>Firmware installieren</button></form>
<p>Anmeldung: Benutzer admin, Passwort = persönlicher API-Schlüssel aus dem USB-Terminal. Nur im vertrauenswürdigen lokalen Netz verwenden.</p></html>)HTML";
void setupRoutes() {
  server.collectHeaders("Authorization", "Origin");
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control","no-store");
    if (portal) {
      server.send(200,"text/html; charset=utf-8", "<!doctype html><html lang=de><meta charset=utf-8><h1>BandFunk WLAN einrichten</h1><form method=post action=/provision><label>SSID <input name=ssid maxlength=32 required></label><label>WLAN-Passwort <input name=password type=password maxlength=64></label><button>Speichern und neu starten</button></form><p>API-Schlüssel und AP-Passwort stehen im USB-Terminal (115200 Baud). Portal endet nach 10 Minuten.</p></html>");
    } else { if (!requireAdmin()) return; server.send_P(200,"text/html; charset=utf-8",PAGE); }
  });
  server.on("/provision", HTTP_POST, []() {
    if (!portal || server.client().localIP() != WiFi.softAPIP()) { server.send(403); return; }
    if (server.hasHeader("Origin") && server.header("Origin") != "http://192.168.4.1") { server.send(403); return; }
    String newSSID=server.arg("ssid"), newPassword=server.arg("password");
    if (!newSSID.length() || newSSID.length()>32 || newPassword.length()>64 || (newPassword.length()>0 && newPassword.length()<8)) { server.send(400,"text/plain","WLAN-Daten ungültig"); return; }
    ssid=newSSID; password=newPassword;
    if (!saveConfig()) { server.send(500,"text/plain","Speichern fehlgeschlagen"); return; }
    server.send(200,"text/plain; charset=utf-8","Gespeichert. Jetzt wieder mit dem Heimnetz verbinden."); rebootAt=millis()+1500;
  });
  server.on("/api/status", HTTP_GET,sendStatus);
  server.on("/api/tx",HTTP_POST,[](){
    if (!requireToken()) return;
    if (safeMode || portal || !radioOK || armed() || txRunning) { server.send(409,"text/plain","Funk derzeit gesperrt"); return; }
    StaticJsonDocument<128> d;
    if (server.arg("plain").length()>128 || deserializeJson(d,server.arg("plain")) || !d["enabled"].is<bool>()) { server.send(400); return; }
    txUntil=d["enabled"].as<bool>()?millis()+120000:0;
    server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/api/light",HTTP_POST,[](){
    if (!requireToken()) return;
    if (!txEnabled() || safeMode || !radioOK || armed()) { server.send(409,"text/plain","Experimentelle Funkübertragung zuerst freigeben"); return; }
    StaticJsonDocument<256> d;
    if (server.arg("plain").length()>256 || deserializeJson(d,server.arg("plain"))) { server.send(400); return; }
    for (const char* field:{"group","r","g","b","effect"}) if (!d[field].is<int>()) { server.send(400); return; }
    int group=d["group"],r=d["r"],g=d["g"],b=d["b"],e=d["effect"];
    if (group<1 || group>10 || r<0 || r>15 || g<0 || g>15 || b<0 || b>15 || e<0 || e>15) { server.send(400); return; }
    groups[group-1].r=r; groups[group-1].g=g; groups[group-1].b=b; groups[group-1].effect=e;
    server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/update", HTTP_POST, []() {
    if (!requireAdmin()) return;
    if (!uploadOK) { server.send(400,"text/plain; charset=utf-8","Update abgelehnt: Freigabe, Signatur, Speicher oder Datei prüfen."); return; }
    server.send(200,"text/plain; charset=utf-8","Signatur geprüft. Neustart folgt."); rebootAt=millis()+1500;
  }, []() {
    HTTPUpload& u=server.upload();
    if (u.status==UPLOAD_FILE_START) {
      uploadOK=false; uploadFailed=false;
      uploadAllowed=armed() && !portal && server.authenticate("admin",token.c_str());
      if (!uploadAllowed) return;
      detachInterrupt(digitalPinToInterrupt(DATA)); radioOK=false; strobe(0x36); // SIDLE
      uint32_t space=(ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
      if (!Update.begin(space,U_FLASH)) uploadFailed=true;
    } else if (u.status==UPLOAD_FILE_WRITE && uploadAllowed && !uploadFailed) {
      if (Update.write(u.buf,u.currentSize)!=u.currentSize) uploadFailed=true;
    } else if (u.status==UPLOAD_FILE_END && uploadAllowed) {
      if (!uploadFailed && Update.end(true)) { uploadOK=true; otaUntil=0; }
      else { if (Update.isRunning()) Update.end(false); uploadFailed=true; }
      uploadAllowed=false;
    } else if (u.status==UPLOAD_FILE_ABORTED) {
      if (Update.isRunning()) Update.end(false);
      uploadAllowed=false; uploadFailed=true;
    }
    yield();
  });
  server.onNotFound([](){ server.send(404,"text/plain","Nicht gefunden"); }); server.begin();
}
void setup() {
  Serial.begin(115200); Serial.println();
  pinMode(BUTTON,INPUT_PULLUP); delay(20); bool recovery=digitalRead(BUTTON)==LOW;
  bool rtcOK=ESP.rtcUserMemoryRead(64,reinterpret_cast<uint32_t*>(&boot),sizeof(boot));
  if (!rtcOK || boot.magic!=0xBAADF00D || boot.check!=(boot.magic^boot.count^0xB4F06A11)) boot={0xBAADF00D,0,0};
  ++boot.count; safeMode=boot.count>=3 || recovery; saveBoot();
  WiFi.persistent(false); WiFi.mode(WIFI_STA);
  deviceId=WiFi.macAddress(); deviceId.replace(":",""); deviceId.toLowerCase(); hostname="bandfunk-"+deviceId.substring(6);
  if (!LittleFS.begin()) { Serial.println("Dateisystemfehler: USB-Recovery erforderlich; keine automatische Formatierung."); return; }
  File f=LittleFS.open("/config.json","r");
  if (f) { StaticJsonDocument<512> doc; if (!deserializeJson(doc,f)) { token=doc["token"]|""; ssid=doc["ssid"]|""; password=doc["password"]|""; } f.close(); }
  if (token.length()!=64) { token=randomHex(32); if (!saveConfig()) { Serial.println("Konfiguration nicht speicherbar"); return; } }
  Serial.printf("BandFunk %s\nAPI-Schluessel: %s\n",FW_VERSION,token.c_str());
  Update.installSignature(&otaHash,&otaVerify);
  WiFi.hostname(hostname); WiFi.setAutoReconnect(true);
  if (ssid.length() && !recovery) {
    WiFi.begin(ssid.c_str(),password.c_str()); uint32_t start=millis();
    while (WiFi.status()!=WL_CONNECTED && millis()-start<20000) { delay(50); }
  }
  // Failed existing Wi-Fi does NOT automatically expose provisioning.
  portal=!ssid.length() || recovery;
  if (portal) { apPassword=randomHex(8); WiFi.mode(WIFI_AP_STA); WiFi.softAP(hostname.c_str(),apPassword.c_str()); Serial.printf("Einrichtungs-WLAN: %s\nAP-Passwort: %s\nhttp://192.168.4.1\n",hostname.c_str(),apPassword.c_str()); }
  if (WiFi.status()==WL_CONNECTED && MDNS.begin(hostname)) {
    MDNS.addService("bandfunk","tcp",80); MDNS.addServiceTxt("bandfunk","tcp","id",deviceId); MDNS.addServiceTxt("bandfunk","tcp","api","1");
    Serial.println(WiFi.localIP());
  }
  if (!safeMode && !portal) radioOK=initRadio();
  timer1_isr_init(); timer1_attachInterrupt(txTimer); setupRoutes();
}
void loop() {
  if (txDone) finishTX();
  server.handleClient(); MDNS.update();
  // At most one 82.4 ms packet per 1100 ms: bounded aggregate airtime.
  if (txEnabled() && millis()-lastTx>=1100) startTX();
  if (txRunning) { delay(1); return; }
  if (portal && millis()>PORTAL_WINDOW) { WiFi.softAPdisconnect(true); portal=false; }
  if (digitalRead(BUTTON)==LOW) {
    if (!buttonSince) buttonSince=millis();
    if (!buttonLatched && millis()-buttonSince>=3000) { otaUntil=millis()+OTA_WINDOW; buttonLatched=true; Serial.println("OTA 5 Minuten freigegeben"); }
  } else { buttonSince=0; buttonLatched=false; }
  // Stable runtime clears repeated-crash counter, independently of Wi-Fi.
  if (!bootCleared && millis()>60000) { boot.count=0; saveBoot(); bootCleared=true; }
  for (uint16_t n=0; n<64 && tail!=head; ++n) {
    uint16_t i=tail; Pulse p{durations[i],bool(levels[i])}; tail=(tail+1)&255; decoder.consume(p);
  }
  if (rebootAt && int32_t(millis()-rebootAt)>=0) ESP.restart();
  delay(1);
}
