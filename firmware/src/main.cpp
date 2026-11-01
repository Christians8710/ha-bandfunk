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
#include "radio_tx.h"
#include "control_state.h"
#include "device_page.h"
#include "app_icon.h"
#include "wifi_portal.h"
#include "provision_guard.h"

// GPIO15/D8 stays completely unused: LOW is required at reset.
constexpr uint8_t CS = D1, DATA = D2, BUTTON = D0;
constexpr uint32_t OTA_WINDOW = 300000, PORTAL_WINDOW = 600000;
ESP8266WebServer server(80);
BearSSL::PublicKey otaKey(OTA_PUBLIC_KEY);
BearSSL::HashSHA256 otaHash;
BearSSL::SigningVerifier otaVerify(&otaKey);
UnknownOOK decoder;
String token, ssid, password, apPassword, deviceId, hostname, provisionNonce, adminHA1, webNonce;
struct AdminSession { String key; uint32_t until=0; } sessions[4];
bool adminDefault=true, radioEnabled=true, configDirty=false, configSaveError=false;
bool uploadBusy=false, restoreRadio=false;
uint32_t configChangedAt=0, nextLoginAt=0;
GroupSetting controls[10];
void markConfigDirty() { configDirty=true; configChangedAt=millis(); }
void syncGroups();
BandFunkWiFiManager wifiManager;
std::unique_ptr<WiFiManagerParameter> portalKeyParameter;
String portalParameterHTML;
String portalState="ready", portalMessage="WLAN auswählen und Passwort eingeben.";
bool portalSaved=false, saveInFlight=false, servicesStarted=false;
uint32_t portalStarted=0, portalConnectedAt=0;
// Default hardware is the board's RESET button. GPIO16/D0 has no pull-up.
#ifndef BANDFUNK_EXTERNAL_D0_BUTTON
#define BANDFUNK_EXTERNAL_D0_BUTTON 0
#endif
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
uint32_t lastTx=0, txPackets=0;
uint32_t txAttempts=0, txCompleted=0, txErrors=0, txWaveStarted=0, txDuration=0;
TxCheck txCheck;
volatile uint32_t txLastEdgeUs=0, txWaveEndUs=0, txLateEdges=0, txMaxLateness=0;
volatile bool txRunning=false, txDone=false;
volatile uint16_t waveIndex=0;
uint16_t wave[358];
bool txEnabled() { return radioEnabled; }
void IRAM_ATTR txTimer() {
  uint32_t now=micros();
  uint32_t elapsed=now-txLastEdgeUs;
  uint32_t expected=wave[waveIndex];
  if (elapsed>expected) {
    uint32_t late=elapsed-expected;
    if (late>txMaxLateness) txMaxLateness=late;
    if (late>40) ++txLateEdges;
  }
  txLastEdgeUs=now;
  uint16_t i=++waveIndex;
  if (i>=358) { digitalWrite(DATA,LOW); txWaveEndUs=now; txRunning=false; txDone=true; return; }
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
String adminHash(const String& value) { return server.credentialHash("admin","BandFunk",value); }
bool digestAdmin() { return server.authenticateDigest("admin",adminHA1); }
int sessionIndex() {
  String cookie=server.header("Cookie");
  int start=cookie.indexOf("bf_session=");
  if (start<0 || (start>0 && cookie[start-1]!=' ' && cookie[start-1]!=';')) return -1;
  String value=cookie.substring(start+11); int end=value.indexOf(';');
  if (end>=0) value=value.substring(0,end);
  if (value.length()!=64) return -1;
  for (int i=0;i<4;++i) if (sessions[i].key==value && int32_t(sessions[i].until-millis())>0) return i;
  return -1;
}
bool adminAllowed() { return sessionIndex()>=0 || digestAdmin(); }
bool requireAdmin(bool json=false) {
  if (adminAllowed()) return true;
  if (json) server.send(401,"text/plain; charset=utf-8","Bitte anmelden.");
  else server.requestAuthentication(DIGEST_AUTH,"BandFunk","Anmeldung erforderlich");
  return false;
}
bool validCSRF() { return server.header("X-BandFunk-CSRF")==webNonce || server.arg("csrf")==webNonce; }
bool requireWebWrite() {
  if (!requireAdmin(true)) return false;
  if (validCSRF()) return true;
  server.send(403,"text/plain; charset=utf-8","Seite abgelaufen. Bitte neu laden."); return false;
}
void pageHeaders() {
  server.sendHeader("Cache-Control","no-store"); server.sendHeader("X-Frame-Options","DENY");
  server.sendHeader("Referrer-Policy","no-referrer"); server.sendHeader("X-Content-Type-Options","nosniff");
}
void redirect(const char* path) { server.sendHeader("Location",path); server.send(303,"text/plain",""); }
void loginPage() {
  pageHeaders(); server.chunkedResponseModeStart(200,"text/html; charset=utf-8"); server.sendContent_P(DEVICE_HEAD);
  server.sendContent("<section class='card'><h2>Anmelden</h2><p>Dein lokaler BandFunk-Controller. Startzugang: admin / bandfunk2026. Ein selbst geändertes Passwort hat Vorrang.</p><form action='/login' method='post'><input type='hidden' name='csrf' value='"+webNonce+"'><label>Benutzer<input name='username' value='admin' autocomplete='username' required></label><label>Passwort<input name='password' type='password' autocomplete='current-password' required></label><p><button>Anmelden</button></p></form><p>Nach einer Passwortänderung bitte mit dem neuen Passwort anmelden.</p></section></main></body></html>");
  server.chunkedResponseFinalize();
}
void syncGroups() { for (int i=0;i<10;++i) groups[i]=appliedColor(controls[i]); }
void saveBoot() {
  boot.check = boot.magic ^ boot.count ^ 0xB4F06A11;
  // Offset 64: do not overwrite eboot OTA command at RTC offset zero.
  ESP.rtcUserMemoryWrite(64, reinterpret_cast<uint32_t*>(&boot), sizeof(boot));
}
bool saveConfig() {
  DynamicJsonDocument doc(3072);
  if (!doc.capacity()) return false;
  doc["schema"]=2; doc["token"]=token; doc["ssid"]=ssid; doc["password"]=password;
  doc["admin_ha1"]=adminHA1; doc["admin_default"]=adminDefault; doc["radio_enabled"]=radioEnabled;
  JsonArray list=doc.createNestedArray("control");
  for (const auto& c:controls) { JsonArray a=list.createNestedArray(); a.add(c.r);a.add(c.g);a.add(c.b);a.add(c.brightness);a.add(c.effect);a.add(c.on); }
  if (doc.overflowed()) return false;
  File f=LittleFS.open("/config.new","w"); if (!f) return false;
  size_t written=serializeJson(doc,f); f.flush();
  bool ok=written==measureJson(doc) && !f.getWriteError(); f.close();
  return ok && LittleFS.rename("/config.new","/config.json");
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
struct RadioTXAccess {
  void holdLow() { digitalWrite(DATA,LOW); pinMode(DATA,OUTPUT); }
  bool strobe(uint8_t command) { return ::strobe(command); }
  bool write(uint8_t address,uint8_t value) { return regWrite(address,value); }
  uint8_t read(uint8_t address) { return regRead(address); }
  bool writePA(uint8_t off,uint8_t on) {
    if (!selectRadio()) return false;
    SPI.transfer(0x7E); SPI.transfer(off); SPI.transfer(on); digitalWrite(CS,HIGH); return true;
  }
  bool readPA(uint8_t& off,uint8_t& on) {
    if (!selectRadio()) return false;
    SPI.transfer(0xFE); off=SPI.transfer(0); on=SPI.transfer(0); digitalWrite(CS,HIGH); return true;
  }
  bool waitState(uint8_t wanted,uint8_t& observed) {
    uint32_t start=micros(); uint8_t previous=0xFF;
    do {
      uint8_t current=regRead(0x35);
      // SWRZ020E: require consecutive matching reads of changing status.
      if (current==previous && current!=0xFF) {
        observed=current&0x1F;
        if (observed==wanted) return true;
      }
      previous=current; delayMicroseconds(20);
    } while (micros()-start<10000);
    return false;
  }
};
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
    {0x0A,0x00},{0x0B,0x06},{0x0C,0x00},{0x0D,0x10},{0x0E,0xB0},{0x0F,0x71},
    {0x10,0x67},{0x11,0x32},{0x12,0x30},{0x13,0x22},{0x14,0xF8},
    {0x15,0x00},{0x17,0x00},{0x18,0x18},{0x19,0x16},{0x1B,0x43},{0x1C,0x40},{0x1D,0x91},
    {0x21,0x56},{0x22,0x10},{0x23,0xE9},{0x24,0x2A},{0x25,0x00},{0x26,0x1F},
    {0x2C,0x81},{0x2D,0x35},{0x2E,0x09}};
  for (const auto& r : config) if (!regWrite(r[0],r[1])) return false;
  if (!strobe(0x34)) return false; // SRX; never STX
  attachInterrupt(digitalPinToInterrupt(DATA),onEdge,CHANGE); return true;
}
void finishTX() {
  timer1_disable();
  bool completed=txDone && waveIndex==358;
  digitalWrite(DATA,LOW);
  // The state check precedes SIDLE and must not disturb the timed waveform.
  RadioTXAccess radio; uint8_t state=0xFF;
  bool stillTX=radio.waitState(0x13,state);
  if (completed && stillTX) {
    ++txCompleted; txDuration=txWaveEndUs-txWaveStarted;
  } else {
    ++txErrors; radioEnabled=false; markConfigDirty();
    txCheck.error=completed?"CC1101 hat TX vorzeitig verlassen":"Impulsfolge hat Zeitlimit erreicht";
  }
  txRunning=false;
  bool idle=strobe(0x36) && radio.waitState(0x01,state);
  if (idle) pinMode(DATA,INPUT);
  if (!idle || !regWrite(0x02,0x0D) || !strobe(0x34)) {
    radioOK=false; radioEnabled=false; markConfigDirty(); txCheck.error="Rueckkehr in Empfang fehlgeschlagen";
  } else {
    attachInterrupt(digitalPinToInterrupt(DATA),onEdge,CHANGE);
  }
  txDone=false;
}
void startTX() {
  if (!radioOK || safeMode || portal || uploadBusy || txRunning || txDone) return;
  uint8_t packet[22]; buildPacket(groups,packet); buildWave(packet,wave);
  detachInterrupt(digitalPinToInterrupt(DATA));
  ++txAttempts;
  RadioTXAccess radio;
  // Release the receiver output before the ESP drives this shared pin.
  if (!prepareRadioTX(radio,txCheck)) {
    ++txErrors; radioEnabled=false; markConfigDirty(); radioOK=false;
    strobe(0x36); pinMode(DATA,INPUT); return;
  }
  waveIndex=0; txRunning=true; txWaveEndUs=0;
  txWaveStarted=micros(); txLastEdgeUs=txWaveStarted; digitalWrite(DATA,HIGH);
  timer1_enable(TIM_DIV16,TIM_EDGE,TIM_SINGLE); timer1_write(uint32_t(wave[0])*5);
  ++txPackets; lastTx=millis();
}
void sendStatus() {
  // HTTP handlers already use the small ESP8266 continuation stack.
  // Keep the expanded diagnostic document on the heap.
  DynamicJsonDocument doc(4096);
  if (!doc.capacity()) { server.send(503,"text/plain","Zu wenig Diagnosespeicher"); return; }
  doc["api"]=1; doc["id"]=deviceId; doc["version"]=FW_VERSION;
  doc["protocol"]=decoder.name(); doc["rssi"]=WiFi.RSSI();
  doc["uptime"]=millis()/1000; doc["free_heap"]=ESP.getFreeHeap();
  doc["radio_ok"]=radioOK; doc["safe_mode"]=safeMode; doc["ota_armed"]=armed();
  doc["edges"]=edgeCount; doc["dropped"]=dropped;
  doc["protocol"]="crowdsync_rf_experimentell";
  doc["tx_enabled"]=txEnabled(); doc["tx_active"]=txEnabled() && radioOK && !safeMode && !portal && !uploadBusy;
  doc["control_api"]=2; doc["config_saved"]=!configDirty; doc["config_error"]=configSaveError;
  doc["admin_default"]=adminDefault; doc["tx_packets"]=txPackets;
  doc["tx_attempts"]=txAttempts; doc["tx_completed"]=txCompleted; doc["tx_errors"]=txErrors;
  doc["tx_config_verified"]=txCheck.configVerified;
  doc["tx_idle_state"]=txCheck.idleState; doc["tx_chip_state"]=txCheck.txState;
  doc["tx_fscal1"]=txCheck.calibration; doc["tx_error"]=txCheck.error;
  doc["tx_mismatch_register"]=txCheck.mismatchAddress;
  doc["tx_expected_value"]=txCheck.expectedValue; doc["tx_actual_value"]=txCheck.actualValue;
  doc["tx_wave_us"]=txDuration; doc["tx_late_edges"]=txLateEdges; doc["tx_max_late_us"]=txMaxLateness;
  JsonArray list=doc.createNestedArray("groups");
  for (const auto& g:groups) { JsonArray a=list.createNestedArray(); a.add(g.r); a.add(g.g); a.add(g.b); a.add(g.effect); }
  JsonArray desired=doc.createNestedArray("control");
  for (const auto& c:controls) { JsonArray a=desired.createNestedArray();a.add(c.r);a.add(c.g);a.add(c.b);a.add(c.brightness);a.add(c.effect);a.add(c.on); }
  if (doc.overflowed()) { server.send(503,"text/plain","Diagnosespeicher voll"); return; }
  String body; serializeJson(doc,body); server.send(200,"application/json",body);
}
String keyPanel() {
  return String("<section><h2>Dein API-Schlüssel für Home Assistant</h2><p>Bei der Einrichtung der BandFunk-Integration einmal eintragen. Dies ist nicht das WLAN-Passwort.</p><label>API-Schlüssel <input id=api-key readonly style='width:100%;box-sizing:border-box' value='") + token +
    "'></label><button type=button onclick=\"var k=document.getElementById('api-key');k.select();try{document.execCommand('copy');this.textContent='Kopiert';}catch(e){this.textContent='Markiert – bitte kopieren';}\">API-Schlüssel kopieren</button><p>Geräteadresse im Heimnetz: <strong>http://" + hostname + ".local/</strong></p></section>";
}
void handleTX(bool web) {
  if (!(web?requireWebWrite():requireToken())) return;
  StaticJsonDocument<128> d;
  if (server.arg("plain").length()>128 || deserializeJson(d,server.arg("plain")) || !d["enabled"].is<bool>()) { server.send(400); return; }
  radioEnabled=d["enabled"].as<bool>(); markConfigDirty();
  server.send(200,"application/json","{\"ok\":true}");
}
void handleControl(bool web,bool legacy=false) {
  if (!(web?requireWebWrite():requireToken())) return;
  StaticJsonDocument<384> d;
  if (server.arg("plain").length()>384 || deserializeJson(d,server.arg("plain"))) { server.send(400); return; }
  for (const char* key:{"group","r","g","b","effect"}) if (!d[key].is<int>()) { server.send(400); return; }
  int group=d["group"],r=d["r"],g=d["g"],b=d["b"],effect=d["effect"];
  int maximum=legacy?15:255;
  if (group<1 || group>10 || r<0 || r>maximum || g<0 || g>maximum || b<0 || b>maximum || effect<0 || effect>15) { server.send(400); return; }
  auto& c=controls[group-1];
  if (legacy) {
    if (r || g || b) { c.r=r*17;c.g=g*17;c.b=b*17;c.brightness=255;c.effect=effect;c.on=true; }
    else c.on=false;
  } else {
    if (!d["brightness"].is<int>() || !d["on"].is<bool>()) { server.send(400); return; }
    int brightness=d["brightness"]; if (brightness<0 || brightness>255) { server.send(400); return; }
    c.r=r;c.g=g;c.b=b;c.brightness=brightness;c.effect=effect;c.on=d["on"];
  }
  syncGroups(); markConfigDirty(); server.send(200,"application/json","{\"ok\":true}");
}
void setupRoutes() {
  server.collectHeaders("Authorization","Origin","Cookie","X-BandFunk-CSRF");
  server.on("/assets/icon.png",HTTP_GET,[](){ server.send_P(200,PSTR("image/png"),reinterpret_cast<const char*>(APP_ICON),sizeof(APP_ICON)); });
  server.on("/login",HTTP_GET,loginPage);
  server.on("/login",HTTP_POST,[](){
    pageHeaders();
    if (!validCSRF()) { server.send(403,"text/plain","Bitte Anmeldeseite neu laden."); return; }
    if (int32_t(nextLoginAt-millis())>0) { server.send(429,"text/plain","Bitte kurz warten."); return; }
    if (server.arg("username")!="admin" || adminHash(server.arg("password"))!=adminHA1) {
      nextLoginAt=millis()+1000; server.send(401,"text/plain; charset=utf-8","Benutzer oder Passwort falsch. Zurück zur Anmeldung und erneut versuchen.");return;
    }
    int slot=0;
    for (int i=0;i<4;++i) if (!sessions[i].key.length() || int32_t(sessions[i].until-millis())<=0) { slot=i;break; }
    sessions[slot].key=randomHex(32); sessions[slot].until=millis()+86400000;
    server.sendHeader("Set-Cookie","bf_session="+sessions[slot].key+"; Path=/; HttpOnly; SameSite=Strict; Max-Age=86400"); redirect("/");
  });
  server.on("/logout",HTTP_POST,[](){
    if (!requireWebWrite()) return;
    int i=sessionIndex(); if (i>=0) sessions[i].key="";
    server.sendHeader("Set-Cookie","bf_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");redirect("/login");
  });
  server.on("/",HTTP_GET,[](){
    pageHeaders(); if (!adminAllowed()) { redirect("/login"); return; }
    server.chunkedResponseModeStart(200,"text/html; charset=utf-8"); server.sendContent_P(DEVICE_HEAD);
    server.sendContent("<meta name='bf-csrf' content='"+webNonce+"'>");
    server.sendContent_P(DEVICE_APP); server.sendContent(keyPanel()); server.sendContent_P(DEVICE_SETTINGS);server.chunkedResponseFinalize();
  });
  server.on("/api/status",HTTP_GET,[](){ if (requireToken()) sendStatus(); });
  server.on("/web/status",HTTP_GET,[](){pageHeaders();if (requireAdmin(true)) sendStatus();});
  server.on("/api/tx",HTTP_POST,[](){handleTX(false);});
  server.on("/web/tx",HTTP_POST,[](){handleTX(true);});
  server.on("/api/light",HTTP_POST,[](){handleControl(false,true);});
  server.on("/api/control",HTTP_POST,[](){handleControl(false);});
  server.on("/web/control",HTTP_POST,[](){handleControl(true);});
  server.on("/web/ota-close",HTTP_POST,[](){if(!requireWebWrite())return;otaUntil=0;server.send(200,"application/json","{\"ok\":true}");});
  server.on("/web/password",HTTP_POST,[](){
    if (!requireWebWrite()) return;
    StaticJsonDocument<384> d;
    if (server.arg("plain").length()>384 || deserializeJson(d,server.arg("plain")) || !d["current"].is<const char*>() || !d["password"].is<const char*>()) {server.send(400);return;}
    if (adminHash(d["current"].as<String>())!=adminHA1) {server.send(403,"text/plain","Bisheriges Passwort ist falsch.");return;}
    if (!validAdminPassword(d["password"])) {server.send(400,"text/plain","8 bis 64 druckbare ASCII-Zeichen verwenden.");return;}
    String old=adminHA1;bool oldDefault=adminDefault;
    adminHA1=adminHash(d["password"].as<String>());adminDefault=d["password"].as<String>()=="bandfunk2026";
    if (!saveConfig()) {adminHA1=old;adminDefault=oldDefault;server.send(500,"text/plain","Passwort nicht gespeichert. Bitte erneut versuchen.");return;}
    configDirty=false;configSaveError=false;for(auto& session:sessions)session.key="";
    server.sendHeader("Set-Cookie","bf_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");server.send(200,"application/json","{\"ok\":true}");
  });
  server.on("/update",HTTP_POST,[](){
    if (!requireAdmin(true)) return;
    if (!uploadOK) {server.send(400,"text/plain; charset=utf-8","Update abgelehnt: physische Freigabe, Anmeldung, Seitenschlüssel, Signatur oder Speicher prüfen.");return;}
    server.send(200,"text/plain; charset=utf-8","Signatur geprüft. Neustart folgt.");rebootAt=millis()+1500;
  },[](){
    HTTPUpload& u=server.upload();
    if (u.status==UPLOAD_FILE_START) {
      uploadOK=false;uploadFailed=false;uploadAllowed=armed() && !portal && adminAllowed() && validCSRF();
      if (!uploadAllowed) return;
      if (configDirty && !saveConfig()) {uploadAllowed=false;uploadFailed=true;return;}
      configDirty=false; uploadBusy=true;timer1_disable();digitalWrite(DATA,LOW);txRunning=false;txDone=false;
      detachInterrupt(digitalPinToInterrupt(DATA));strobe(0x36);radioOK=false;
      uint32_t space=(ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
      if (!Update.begin(space,U_FLASH)) uploadFailed=true;
    } else if (u.status==UPLOAD_FILE_WRITE && uploadAllowed && !uploadFailed) {
      if (Update.write(u.buf,u.currentSize)!=u.currentSize)uploadFailed=true;
    } else if (u.status==UPLOAD_FILE_END && uploadAllowed) {
      if (!uploadFailed && Update.end(true)) {uploadOK=true;otaUntil=0;}
      else {if(Update.isRunning())Update.end(false);uploadFailed=true;restoreRadio=true;}
      uploadAllowed=false;uploadBusy=false;
    } else if (u.status==UPLOAD_FILE_ABORTED) {
      if(Update.isRunning())Update.end(false);
      restoreRadio=uploadBusy;uploadAllowed=false;uploadBusy=false;uploadFailed=true;
    }
    yield();
  });
  server.onNotFound([](){server.send(404,"text/plain","Nicht gefunden");});server.begin();
}
void startDeviceServices() {
  if (servicesStarted) return;
  WiFi.persistent(false); WiFi.setAutoReconnect(true);
  if (WiFi.status()==WL_CONNECTED && MDNS.begin(hostname)) {
    MDNS.addService("bandfunk","tcp",80);
    MDNS.addServiceTxt("bandfunk","tcp","id",deviceId);
    MDNS.addServiceTxt("bandfunk","tcp","api","1");
    Serial.printf("WLAN verbunden. IP: %s\n",WiFi.localIP().toString().c_str());
  }
  if (!safeMode && WiFi.status()==WL_CONNECTED) radioOK=initRadio();
  setupRoutes(); servicesStarted=true;
}
void startPortal() {
  portal=true; portalStarted=millis(); portalSaved=false;
  apPassword="bandfunk2026";
  wifiManager.setDebugOutput(false);
  wifiManager.setRestorePersistent(false);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(PORTAL_WINDOW/1000);
  wifiManager.setConnectTimeout(25);
  wifiManager.setSaveConnectTimeout(25);
  wifiManager.setBreakAfterConfig(false);
  wifiManager.setDisableConfigPortal(false); // Keep success/IP visible for 30 seconds.
  wifiManager.setShowInfoUpdate(false); wifiManager.setShowInfoErase(false);
  wifiManager.setShowPassword(false); wifiManager.setShowStaticFields(false);
  wifiManager.setShowDnsFields(false); wifiManager.setTitle("BandFunk HA " FW_VERSION);
  wifiManager.setHostname(hostname.c_str());
  wifiManager.setClass("invert");
  const char* menu[]={"wifi","wifinoscan"}; wifiManager.setMenu(menu,2);
  wifiManager.setCustomHeadElement("<meta name='referrer' content='no-referrer'><style>body.invert,body.invert a,body.invert h1{background:#10252c}button{background:#f5c66a;color:#10252c}</style>");
  portalParameterHTML="<input type='hidden' name='nonce' value='"+provisionNonce+"'>"+keyPanel();
  portalKeyParameter.reset(new WiFiManagerParameter(portalParameterHTML.c_str()));
  wifiManager.addParameter(portalKeyParameter.get());
  wifiManager.setSaveConfigCallback([](){
    saveInFlight=false;
    if (WiFi.status()!=WL_CONNECTED) return;
    String previousSSID=ssid, previousPassword=password;
    ssid=wifiManager.getWiFiSSID(false); password=wifiManager.getWiFiPass(false);
    if (!saveConfig()) {
      ssid=previousSSID; password=previousPassword;
      portalState="save_failed"; portalMessage="WLAN verbunden, aber Speichern fehlgeschlagen. Bitte erneut versuchen.";
      return;
    }
    portalSaved=true; portalConnectedAt=millis();
    portalState="connected"; portalMessage="WLAN-Verbindung erfolgreich bestätigt";
  });
  wifiManager.setWebServerCallback([](){
    // These first-match handlers prevent library OTA/erase and protect saves.
    auto deny=[](){wifiManager.server->send(403,"text/plain; charset=utf-8","Diese Funktion ist im Einrichtungsportal gesperrt.");};
    for (const char* path:{"/update","/u","/erase","/restart","/exit","/close","/info","/param","/paramsave","/status"})
      wifiManager.server->on(path,HTTP_ANY,deny);
    wifiManager.server->on("/",HTTP_GET,[](){
      if (!wifiManager.fromAP()) { wifiManager.deny(); return; }
      wifiManager.headers();
      String page="<!doctype html><html lang=de><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>BandFunk einrichten</title><h1>BandFunk HA " FW_VERSION "</h1><p>Passwort für das Einrichtungs-WLAN: bandfunk2026</p>";
      page+=keyPanel();page+="<form method='post' action='/admin-reset'><input type='hidden' name='nonce' value='"+provisionNonce+"'><button>Admin-Passwort auf bandfunk2026 zurücksetzen</button></form>"; page+="<p><a href='/wifi'>WLAN auswählen und verbinden</a></p><p><a href='/0wifi'>Verstecktes WLAN manuell eintragen</a></p><p>Nach dem Speichern wird die Verbindung geprüft. Bei Fehlern bitte wieder mit dem Einrichtungs-WLAN verbinden.</p></html>";
      wifiManager.server->send(200,"text/html; charset=utf-8",page);
    });
    wifiManager.server->on("/admin-reset",HTTP_POST,[](){
      if(!wifiManager.checkSave(provisionNonce)) {wifiManager.deny();return;}
      String old=adminHA1;bool oldDefault=adminDefault;adminHA1=adminHash("bandfunk2026");adminDefault=true;
      if(!saveConfig()) {adminHA1=old;adminDefault=oldDefault;wifiManager.server->send(500,"text/plain","Speichern fehlgeschlagen");return;}
      for(auto& session:sessions)session.key="";
      wifiManager.server->send(200,"text/plain; charset=utf-8","Admin-Passwort ist wieder bandfunk2026. WLAN und HA-Schlüssel bleiben erhalten.");
    });
    wifiManager.server->on("/wifi",HTTP_ANY,[](){wifiManager.wifiPage(true);});
    wifiManager.server->on("/0wifi",HTTP_ANY,[](){wifiManager.wifiPage(false);});
    wifiManager.server->on("/wifisave",HTTP_ANY,[](){
      if (!wifiManager.checkSave(provisionNonce)) { wifiManager.deny(); return; }
      if (!wifiManager.server->arg("s").length() || wifiManager.server->arg("s").length()>32 || wifiManager.server->arg("p").length()>64) {
        wifiManager.server->send(400,"text/plain; charset=utf-8","Bitte gültige SSID und WLAN-Passwort eingeben."); return;
      }
      saveInFlight=true; portalSaved=false; portalState="connecting"; portalMessage="WLAN-Verbindung wird geprüft …";
      wifiManager.saveWiFi();
    });
    wifiManager.server->on("/bandfunk-status",HTTP_GET,[](){
      if (!wifiManager.fromAP()) { wifiManager.deny(); return; }
      wifiManager.headers(); StaticJsonDocument<512> doc;
      doc["state"]=portalState; doc["message"]=portalMessage;
      doc["ip"]=portalSaved?WiFi.localIP().toString():"";
      String body; serializeJson(doc,body); wifiManager.server->send(200,"application/json",body);
    });
  });
  wifiManager.startConfigPortal(hostname.c_str(),apPassword.c_str());
  Serial.printf("WLAN-Einrichtung mit WiFiManager: %s / http://192.168.4.1\n",hostname.c_str());
}
void setup() {
  Serial.begin(115200); Serial.println();
  bool recovery=false;
#if BANDFUNK_EXTERNAL_D0_BUTTON
  pinMode(BUTTON,INPUT); delay(20); recovery=digitalRead(BUTTON)==LOW;
#endif
  bool rtcOK=ESP.rtcUserMemoryRead(64,reinterpret_cast<uint32_t*>(&boot),sizeof(boot));
  if (!rtcOK || boot.magic!=0xBAADF00D || boot.check!=(boot.magic^boot.count^0xB4F06A11)) boot={0xBAADF00D,0,0};
  ++boot.count; safeMode=boot.count>=3 || recovery; saveBoot();
  WiFi.persistent(false); WiFi.mode(WIFI_STA);
  deviceId=WiFi.macAddress(); deviceId.replace(":",""); deviceId.toLowerCase(); hostname="bandfunk-"+deviceId.substring(6);
  if (!LittleFS.begin()) { Serial.println("Dateisystemfehler: USB-Recovery erforderlich; keine automatische Formatierung."); return; }
  bool migrated=false;
  File f=LittleFS.open("/config.json","r");
  if (f) {
    DynamicJsonDocument doc(3072);
    if (!deserializeJson(doc,f)) {
      token=doc["token"]|"";ssid=doc["ssid"]|"";password=doc["password"]|"";
      adminHA1=doc["admin_ha1"]|"";adminDefault=doc["admin_default"]|true;
      radioEnabled=doc["radio_enabled"]|true;
      JsonArray list=doc["control"].as<JsonArray>();
      if (list.size()==10) for(int i=0;i<10;++i) {
        JsonArray a=list[i]; if(a.size()!=6)continue;
        bool valid=a[5].is<bool>();for(int j=0;j<5;++j)valid=valid && a[j].is<int>() && a[j].as<int>()>=0 && a[j].as<int>()<=(j==4?15:255);
        if(valid) {auto& c=controls[i];c.r=a[0];c.g=a[1];c.b=a[2];c.brightness=a[3];c.effect=a[4];c.on=a[5];}
      }
    }
    f.close();
  }
  if(token.length()!=64) {token=randomHex(32);migrated=true;}
  if(adminHA1.length()!=32) {adminHA1=adminHash("bandfunk2026");adminDefault=true;migrated=true;}
  if(migrated && !saveConfig()) {Serial.println("Konfiguration nicht speicherbar");return;}
  syncGroups();webNonce=randomHex(16);
  Serial.printf("BandFunk %s\n",FW_VERSION);
  Update.installSignature(&otaHash,&otaVerify);
  WiFi.hostname(hostname); WiFi.setAutoReconnect(true);
  if (ssid.length() && !recovery) {
    WiFi.begin(ssid.c_str(),password.c_str()); uint32_t start=millis();
    while (WiFi.status()!=WL_CONNECTED && millis()-start<20000) { delay(50); }
  }
  // External RESET opens a physical OTA window. Cold power-on does not.
  if (ESP.getResetInfoPtr()->reason==REASON_EXT_SYS_RST) otaUntil=millis()+OTA_WINDOW;
  provisionNonce=randomHex(16);
  timer1_isr_init(); timer1_attachInterrupt(txTimer);
  if (WiFi.status()!=WL_CONNECTED || recovery || safeMode) startPortal();
  else startDeviceServices();
}

void loop() {
  if (!bootCleared && millis()>60000) { boot.count=0; saveBoot(); bootCleared=true; }
  if (portal) {
    wifiManager.process();
    // process() finishes a submitted connection attempt before returning.
    if (saveInFlight) {
      saveInFlight=false; portalState="failed";
      uint8_t result=wifiManager.getLastConxResult();
      portalMessage=result==WL_NO_SSID_AVAIL?"WLAN nicht gefunden. Bitte Auswahl prüfen.":
        "Verbindung fehlgeschlagen. Bitte Passwort und 2,4-GHz-WLAN prüfen und erneut versuchen.";
    }
    bool finished=portalSaved && millis()-portalConnectedAt>=30000;
    if (finished || millis()-portalStarted>=PORTAL_WINDOW) {
      wifiManager.stopConfigPortal(); portal=false;
      WiFi.mode(WIFI_STA); WiFi.persistent(false);
      startDeviceServices();
    }
    delay(1); return;
  }
  if (txDone || (txRunning && uint32_t(micros()-txWaveStarted)>150000)) finishTX();
  server.handleClient(); MDNS.update();
  if(restoreRadio && !uploadBusy) {restoreRadio=false;if(!safeMode)radioOK=initRadio();}
  // At most one 82.4 ms packet per 1100 ms: bounded aggregate airtime.
  if (txEnabled() && millis()-lastTx>=1100) startTX();
  if (txRunning) { delay(1); return; }
  if(configDirty && !uploadBusy && millis()-configChangedAt>=3000) {
    configSaveError=!saveConfig();if(!configSaveError)configDirty=false;else configChangedAt=millis();
  }
#if BANDFUNK_EXTERNAL_D0_BUTTON
  if (digitalRead(BUTTON)==LOW) {
    if (!buttonSince) buttonSince=millis();
    if (!buttonLatched && millis()-buttonSince>=3000) { otaUntil=millis()+OTA_WINDOW; buttonLatched=true; }
  } else { buttonSince=0; buttonLatched=false; }
#endif
  for (uint16_t n=0; n<64 && tail!=head; ++n) {
    uint16_t i=tail; Pulse p{durations[i],bool(levels[i])}; tail=(tail+1)&255; decoder.consume(p);
  }
  if (rebootAt && int32_t(millis()-rebootAt)>=0) ESP.restart();
  delay(1);
}
