# Hardware-Testprotokoll V0.1.0

Stand: 1. November 2026. **Erste Tests erfolgt, RGB weiterhin nicht bestätigt.**

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

## Beobachteter Rot-Test mit 0.1.4

Home Assistant erreicht den ESP; CC1101 wird erkannt. Funkfreigabe aktiv, Recovery und OTA nicht aktiv. Gruppen 2 und 6 auf Dauerlicht/Rot gesetzt; wiederholte Pakete im Abstand von 1,1 Sekunden. Der Softwarezähler steigt. Besitzer bestätigt GDO0 an D2 und angeschlossene Antenne. Nach Herausnehmen und Wiedereinsetzen der Armband-Batterien bleiben beide Bänder dunkel. Ausgangsleistung, tatsächliche Funkwellenform und Empfang am Band wurden dabei nicht gemessen. Damit ist weder ein Funkdefekt noch ein abweichendes V5-Protokoll bewiesen.

## Zusätzliche Prüfungen ab 0.1.5

- Register-/PATABLE-Rücklesen, IDLE und TX (`MARCSTATE=0x13`) sowie FSCAL1-Kalibrierung beim ersten Test prüfen.
- `tx_packets`: Chip hat vor Impulsausgabe TX gemeldet; keine Empfangsbestätigung.
- `tx_completed`: 358 Zeitintervalle beendet und Chip am Ende weiterhin in TX; Sollzeit 82.400 µs, interne Verspätungszähler berücksichtigen.
- `tx_errors` und `tx_error`: Fehler führen zur Abschaltung der Funkfreigabe. Timer-Zeitlimit 150 ms. Danach Geräte-Neustart bei verriegeltem Funkfehler.
- `tx_late_edges` zählt interne Intervallverspätungen über 40 µs seit Neustart; `tx_max_late_us` zeigt das Maximum. Interne Timerwerte ersetzen keine Messung am GDO0 oder Antennenanschluss.
- Nach OTA bisherige WLAN-/API-Daten prüfen; HA 0.1.4 kann die zusätzlichen API-1-Felder weiterhin lesen.

## Gerätetest 0.1.5 bis 0.1.7

- 0.1.5: Signiertes OTA akzeptiert; danach zwei `Exception (2)` im seriellen Startprotokoll. Automatisierte Softwaretests hatten diesen Gerätefehler nicht erkannt. Version zurückgezogen und Installationsknopf deaktiviert.
- 0.1.6: Status-JSON vom ESP-Stack auf Heap verlegt. Signiertes OTA erfolgreich, normale WLAN-Verbindung und bestehender API-Schlüssel bestätigt. Build prüft jetzt den realen Stackrahmen von `sendStatus` (lokal 128 Byte, maximal 1024). Beim Sendetest Registerabweichung vor erster bestätigter TX-Aktivierung erkannt.
- 0.1.7: Signiertes OTA erfolgreich. 20 API-Abfragen ohne beobachteten Neustart; dabei Betriebszeit 82 s. Diagnose: `tx_attempts=1`, `tx_packets=0`, `tx_completed=0`, `tx_errors=1`, IDLE bestätigt (`1`), erste Registerabweichung bei Adresse `2` (`IOCFG0`): Soll `46` (`0x2E`), Ist `15` (`0x0F`). Funkfreigabe wird geschlossen. Ergebnis ist ein nicht passender SPI-Rücklesewert, noch kein Beweis einer bestimmten Hardwareursache und kein Protokolltest über Funk.
- Home-Assistant-Eintrag während der Reparatur vorübergehend deaktiviert und anschließend wieder aktiviert. Integration bleibt API-1-kompatibel.

### Ergänzende Hardwarequelle

[Wristbanditø-Dokument](https://docs.google.com/document/d/1kwlhJLc4ZCnrCcFRf51oEpflG-rDBOV7JPWpBCfqqds/edit), verlinkt im [Reddit-Originalbeitrag](https://www.reddit.com/r/twentyonepilots/comments/1nmtt2q/wristbandit%C3%B8_reverse_engineering_the_twenty_one/), abgerufen am 1. November 2026: Foto zeigt ebenfalls `23A033V5`, Gruppe 2. Autor beschreibt CMT2210LH, 27,1412-MHz-Quarz, 3,3-V-LDO und parallele RGB-LEDs. Der Text erklärt das Funkformat ausdrücklich für unbekannt; MCU-Typ nur vermutet. Keine zusätzliche Paketdefinition, kein fertiger Sender und keine V5-Protokollbestätigung daraus ableiten. Fremde Fotos/Schaltpläne wurden nicht ins Projekt kopiert.

Diagnosegrundlagen: [TI-Datenblatt SWRS061I](https://www.ti.com/lit/ds/symlink/cc1101.pdf), MARCSTATE/FSCAL1/Asynchronmodus; [TI-Errata SWRZ020E](https://www.ti.com/lit/er/swrz020e/swrz020e.pdf), wiederholte Statuslesung und PLL-Prüfung über FSCAL1.

## Zusätzliche Prüfungen ab 0.1.4

- Ohne D0-Beschaltung erfolgreich ins Heim-WLAN wechseln und Kaltstart wiederholen.
- Falsches WLAN-Passwort: keine Erfolgsbestätigung; AP bleibt für erneute Eingabe offen.
- Richtiges Passwort: DHCP-IP sichtbar, AP nach 30 Sekunden aus, Geräte-Webseite und HA-Discovery erreichbar.
- Nach normalem Neustart dieselbe WLAN-Verbindung und denselben API-Schlüssel prüfen.
- WiFiManager `/u` und `/update` auch per direktem POST testen: 403, kein Flash-Schreiben.
- Fehlender/falscher Formularschlüssel und Zugriff über LAN statt AP: Speichern abgewiesen.
- RESET öffnet OTA-Fenster; kalter Stromstart tut das nicht. Fenster über Geräte-Webseite schließen; Funk dann wieder freigebbar.
