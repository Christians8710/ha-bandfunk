# Hardware-Testprotokoll V0.1.0

Stand: 1. November 2026. **Noch keine Hardwareprüfung erfolgt.**

| Armband | Sichtbare Platine | Gehäusegruppe | Protokoll bestätigt |
|---|---|---|---|
| Band 1 | 23A033V5 | 6 | nein |
| Band 2 | 23A033V5 | 2 | nein |

Datum, Tester, ESP-Modell/Flashgröße, E07-Modellzusatz, Verdrahtung, Antenne, Versorgung und Firmware-SHA256 dokumentieren. Private WLAN-/API-Daten nicht eintragen.

| Prüfung | Erwartung | Ergebnis |
|---|---|---|
| 10 Kaltstarts | zuverlässiger Normalstart, D8 LOW | offen |
| Modul abgezogen | HA erreichbar, Funkfehler statt Startblockade | offen |
| WLAN falsch / nicht verfügbar | geschützter Einrichtungs-AP, nach 10 Minuten aus | offen |
| 3 RESET-Neustarts | WPA2-Portal, kein D0-Zusatztaster nötig, Timeout nach 10 Minuten | offen |
| mDNS / manuelle IP | eindeutiger Eintrag, kein Duplikat | offen |
| Gruppen 6 und 2, jeweils R/G/B/Aus | korrekte Farbe nur in gewählter Gruppe, andere bleibt erhalten | offen |
| LED-Aus nach Schwarz | innerhalb der Empfangsfenster, vor Testende | offen |
| Timer-Wellenform am GDO0 | 120/320 µs, Sync 1600 µs, MSB zuerst, Schlussflanke | offen |
| CC1101 RF-Leistung / Spektrum | erlaubtes Band, niedrige Leistung, keine Dauerträger | offen |
| Funkfreigabeablauf | nach 120 Sekunden kein weiteres Paket | offen |
| OTA ohne Taster / falsches Passwort | abgewiesen | offen |
| Unsigned / falscher Schlüssel / manipuliertes Image | abgewiesen, bisheriger Start bleibt möglich | offen |
| Gültig signiertes Image | erfolgreicher Neustart, WLAN/API erhalten | offen |
| Uploadabbruch | keine Aktivierung, gültiges Image danach installierbar | offen |
| Stromausfall während Upload / Kopieren | auf Testgerät prüfen; USB-Recovery bereithalten | offen |
| 3 schnelle Resets / eine Minute stabil | Recovery ohne Funk, danach Zähler zurückgesetzt | offen |
| Diagnosedownload | keine Zugangsdaten, IP/MAC/SSID enthalten | offen |

Automatisierte Tests ersetzen diese Prüfungen nicht. Negative Befunde mit Ablauf, Soll-/Istverhalten und Firmware-SHA melden.

## Zusätzliche Prüfungen ab 0.1.4

- Ohne D0-Beschaltung erfolgreich ins Heim-WLAN wechseln und Kaltstart wiederholen.
- Falsches WLAN-Passwort: keine Erfolgsbestätigung; AP bleibt für erneute Eingabe offen.
- Richtiges Passwort: DHCP-IP sichtbar, AP nach 30 Sekunden aus, Geräte-Webseite und HA-Discovery erreichbar.
- Nach normalem Neustart dieselbe WLAN-Verbindung und denselben API-Schlüssel prüfen.
- WiFiManager `/u` und `/update` auch per direktem POST testen: 403, kein Flash-Schreiben.
- Fehlender/falscher Formularschlüssel und Zugriff über LAN statt AP: Speichern abgewiesen.
- RESET öffnet OTA-Fenster; kalter Stromstart tut das nicht. Fenster über Geräte-Webseite schließen; Funk dann wieder freigebbar.
