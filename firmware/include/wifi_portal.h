#pragma once
#include <WiFiManager.h>
#include "provision_guard.h"

// Uses the upstream WiFiManager connection/scan/DNS implementation unchanged.
// Register guarded routes before upstream routes (ESP8266WebServer first match).
class BandFunkWiFiManager final : public WiFiManager {
 public:
  void wifiPage(bool scan) {
    if (!fromAP()) { deny(); return; }
    headers(); handleWifi(scan);
  }
  bool fromAP() const { return server && server->client().localIP()==WiFi.softAPIP(); }
  void headers() {
    server->sendHeader("Cache-Control","no-store");
    server->sendHeader("X-Frame-Options","DENY");
    server->sendHeader("Referrer-Policy","no-referrer");
  }
  void deny() { server->send(403,"text/plain; charset=utf-8","Nur im aktiven Einrichtungs-WLAN verfügbar."); }
  bool checkSave(const String& nonce) {
    return server->method()==HTTP_POST &&
      allowProvision(getConfigPortalActive(),fromAP(),nonce.c_str(),server->arg("nonce").c_str());
  }
  void saveWiFi() { headers(); handleWifiSave(); }
};
