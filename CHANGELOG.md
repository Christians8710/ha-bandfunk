# Änderungen

## 0.1.4

WLAN-Einrichtung auf tzapu/WiFiManager 2.0.17 umgestellt: geprüfter Verbindungsversuch, Statusanzeige mit Geräte-IP, erneute Eingabe bei Fehlern und AP-Abschaltung nach bestätigtem Erfolg. Deutsche Templates und API-Schlüssel-Anzeige bleiben erhalten. Onboard-RESET statt unbeschaltetem GPIO16/D0 als physische OTA-Freigabe; zusätzlicher D0-Taster ist opt-in. Bibliotheks-OTA und administrative Nebenwege sind gesperrt.


## 0.1.3

Behebt HTTP 403 beim Speichern der WLAN-Daten: AP-gebundenes Formular mit zufälligem Nonce pro Neustart statt starrer Origin-Prüfung. Funktioniert auch bei browserseitig unterdrückter Herkunft. Abgelaufene Formulare zeigen einen verständlichen Hinweis. Native Regressionstests prüfen gültige, fehlende und falsche Nonces sowie die AP-/Portal-Sperren.

## 0.1.2

Einrichtungsseite zeigt nahe 2,4-GHz-WLANs mit Signalstärke und Verschlüsselungsstatus. Asynchrone Suche, erneuter Scan, Auswahl und manuelle Eingabe versteckter SSIDs. Ergebnisse bleiben lokal im Einrichtungs-WLAN; kein Upload von WLAN-Namen.

## 0.1.1

Einrichtungs-WLAN verwendet auf Wunsch des Besitzers das feste Passwort `bandfunk2026` (alles klein). Der persönliche API-Schlüssel bleibt gerätespezifisch. API-Schlüssel ist auf der Einrichtungsseite und später auf der angemeldeten Geräte-Webseite mit Kopierknopf sichtbar. Mit demselben OTA-Schlüssel signiert; kein Flash-Löschen nötig.

## 0.1.0

Erste deutsche HACS-Integration, Web-Flasher für lokalen PC, WPA2-WLAN-Provisioning, signiertes physisch freigegebenes OTA, Recovery, Diagnostik und experimentelle RGB-Gruppensteuerung nach dokumentierter Protokollreferenz. V5-Armbänder Gruppe 6 und 2 noch nicht hardwarebestätigt.
