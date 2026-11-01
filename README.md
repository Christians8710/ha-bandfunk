# BandFunk HA · Integration V0.1.8 / Firmware V0.1.7

![BandFunk – Armband mit Funkwellen](web/assets/bandfunk-logo-v1.png)

Deutsche HACS-Custom-Integration und ESP8266-Firmware für Wemos/LOLIN D1 mini (4 MB) mit EBYTE E07-M1101D-TH bzw. passendem SMA-Modul, CC1101 bei 433,92 MHz.

**Experimentelle RGB-Steuerung ist implementiert, aber noch nicht an den Armbändern des Besitzers bestätigt.** Grundlage ist die öffentlich dokumentierte und dort funkgetestete Variante von [James Wang / CrowdSync_RF](https://github.com/jamesw343/CrowdSync_RF/tree/b800899687224011657e4ac62caaf45d54892f6d). Diese Referenz betrifft `23A033V7_2`, Gruppe 6. Die vorliegenden Armbänder sind `23A033V5`: Band 1 trägt Gruppe 6, Band 2 Gruppe 2. Fotos belegen die Kennzeichnungen, nicht die Protokollkompatibilität. Keine Hersteller-Firmware oder privaten Bilder sind Bestandteil dieses Repositories.

BandFunk ist unabhängig und weder vom CrowdSync-Hersteller noch vom Home-Assistant-Projekt beauftragt oder unterstützt. Fremde Namen dienen der technischen Zuordnung. Kein Herstellerlogo wird übernommen.

## Funktionen

- HACS-Installation, deutscher Config Flow, manuelle IP/Hostname-Einrichtung und mDNS-Discovery.
- Zehn experimentelle Gruppen-Lichtentitäten mit RGB (4 Bit pro Kanal), Helligkeit und 16 dokumentierten Effekten. Zustände sind **gesendete Sollwerte**, keine Empfangsbestätigung.
- Bewusste Funkfreigabe über einen HA-Schalter, jeweils für zwei Minuten; nach Neustart ausgeschaltet. Der Testbetrieb endet automatisch, eine stehengebliebene LED wird dadurch nicht automatisch ausgeschaltet.
- Alle zehn Gruppen werden gemeinsam übertragen. Bei Änderungen bleiben die anderen Gruppen-Sollwerte im RAM erhalten. Nach ESP-Neustart sind alle schwarz. Mehrere Armbänder derselben Gruppe sind nicht individuell adressierbar.
- WLAN-Einrichtung über zeitlich begrenztes WPA2-AP mit festem Passwort `bandfunk2026`. Keine Improv-Serial-Provisionierung: ESP Web Tools dient hier dem USB-Flash und Terminal, WLAN wird über das Geräteportal eingerichtet.
- OTA mit RSA-2048/SHA-256-Signaturprüfung, Digest-Anmeldung, zusätzlichem physischem Freigabefenster, Statusdiagnostik und Recovery-Modus.
- Keine Cloud-Verbindung für den laufenden Betrieb. Die lokale API nutzt HTTP mit Bearer-Schlüssel und ist nur für ein vertrauenswürdiges LAN vorgesehen; sie ist nicht Ende-zu-Ende verschlüsselt.

## Verkabelung — zuerst prüfen

**Nur 3,3 V**, niemals CC1101 an 5 V anschließen. Gemeinsame Masse, kurze Leitungen, 100 nF plus 10 µF am Funkmodul. Vor dem Umstecken Versorgung trennen. Passende 433-MHz-Antenne verwenden.

| E07-M1101D-TH Hersteller-Pin | Signal | D1 mini |
|---|---|---|
| 1 | GND | G |
| 2 | VCC | 3V3 |
| 3 | GDO0 | D2 / GPIO4, asynchrones RX/TX-Datensignal |
| 4 | CSN | D1 / GPIO5, zusätzlich 10 kΩ nach 3V3 empfohlen |
| 5 | SCK | D5 / GPIO14 |
| 6 | MOSI | D7 / GPIO13 |
| 7 | MISO/GDO1 | D6 / GPIO12 |
| 8 | GDO2 | nicht verbunden |

Der vorhandene **RESET-Taster** öffnet nach einem kurzen Druck ein fünfminütiges OTA-Fenster. D0 bleibt standardmäßig unbenutzt: GPIO16 hat keinen internen Pull-up und darf ohne Zusatzbeschaltung nicht als gedrückter Taster interpretiert werden. Ein optionaler Zusatztaster D0–GND mit externem 10-kΩ-Pull-up nach 3V3 kann beim Selbstbau über `BANDFUNK_EXTERNAL_D0_BUTTON=1` aktiviert werden.

**D8 / GPIO15 bleibt frei.** GPIO15 muss beim Reset LOW sein; CSN benötigt dagegen einen inaktiven HIGH-Pegel. Ein CSN-Pull-up an D8 kann den ESP-Start verhindern, ein Pulldown kann den CC1101 während des Bootens selektieren. Deshalb nutzt dieses Projekt D1. Auch D3/GPIO0 und D4/GPIO2 werden nicht für das Funkmodul benutzt.

Die Zahlen beziehen sich auf die [offizielle EBYTE-TH-Tabelle](https://www.ebyte.com/product/58.html), nicht auf eine beliebige Internetzeichnung. SMA-Varianten können zusätzliche Massepins besitzen. Gehäuseansicht, Pin-1-Markierung und Modellzusatz mit der Zeichnung des tatsächlich gelieferten Moduls abgleichen. „E07-M1101D“ ohne Zusatz reicht nicht zur sicheren geometrischen Zuordnung. [LOLIN-Dokumentation](https://www.wemos.cc/en/latest/d1/d1_mini.html), [TI CC1101-Datenblatt](https://www.ti.com/lit/ds/symlink/cc1101.pdf).

## Integrationslogo

Das BandFunk-Symbol wird ab Home Assistant 2026.3 lokal aus `custom_components/bandfunk/brand/` geladen, auch auf dunklen Oberflächen. Für ältere HA-Versionen bleiben die Funktionen verfügbar, die lokale Logoanzeige ist dort jedoch nicht unterstützt. Nach einer HACS-Aktualisierung gegebenenfalls HA neu starten und den Browser neu laden. Kein Firmware-Flash für das Logo erforderlich.

## Installation

1. In HACS → Menü → Benutzerdefinierte Repositories: `https://github.com/Christians8710/ha-bandfunk`, Typ **Integration** hinzufügen.
2. **BandFunk HA** herunterladen, Home Assistant neu starten.
3. [HTTPS-Web-Flasher](https://Christians8710.github.io/ha-bandfunk/) am PC mit dem angeschlossenen ESP öffnen. Desktop-Chrome oder Edge mit Web Serial verwenden. Zugriff erfolgt auf den COM-Port **dieses PCs**, nicht auf den HA-Server. Eventuell CH340-Treiber und ein echtes USB-Datenkabel erforderlich.
4. Einmalig `bandfunk-0.1.7.bin` flashen. Mit dem Einrichtungs-WLAN `bandfunk-…` und Passwort `bandfunk2026` verbinden. In Chrome `http://192.168.4.1` öffnen: Dort steht der persönliche API-Schlüssel mit Kopierknopf. Vor dem Speichern der WLAN-Daten privat sichern. Der API-Schlüssel wird pro Gerät erzeugt; das Einrichtungs-WLAN-Passwort ist bewusst fest vorgegeben.
5. Mit `bandfunk-…` verbinden, `http://192.168.4.1` öffnen, WLAN aus der Liste auswählen und Passwort eingeben. Die Suche zeigt 2,4-GHz-Netze am Standort des ESP; über „Neu suchen“ aktualisieren. Versteckte SSIDs können manuell eingegeben werden. **Speichern und verbinden** wählen und bis zu 30 Sekunden auf die bestätigte Verbindung und neue Geräte-IP warten. Der AP bleibt nach Erfolg noch 30 Sekunden für die Anzeige offen. Bei falschem Passwort bleibt er für einen weiteren Versuch erreichbar. Anschließend wieder mit dem Heim-WLAN verbinden und die angezeigte Geräte-IP öffnen. Die Einrichtung verwendet [WiFiManager 2.0.17](https://github.com/tzapu/WiFiManager/tree/v2.0.17), mit deutschen Templates, Captive DNS und manueller SSID-Eingabe. Wenn beim Start keine Verbindung gelingt, startet ein auf zehn Minuten begrenzter Einrichtungs-AP.
6. HA → Einstellungen → Geräte & Dienste → Integration hinzufügen → **BandFunk HA**. Host und API-Schlüssel eingeben oder mDNS-Fund auswählen. Der Schlüssel bleibt später auch auf der lokalen Geräte-Webseite sichtbar: Anmeldung als `admin`, Passwort ist der API-Schlüssel. Diese Anzeige funktioniert in Chrome und bleibt vor anonymem Zugriff aus dem Heimnetz geschützt. Discovery benötigt ein gemeinsames Multicast-Netz; andernfalls IP manuell eintragen. DHCP-Reservierung empfohlen.

## Erster RGB-Test mit den zwei Armbändern

1. Frische Batterien, Polung prüfen. Keine Kontakte am Armband überbrücken oder umlöten.
2. CC1101-Diagnose muss erkannt sein, Recovery und OTA-Fenster müssen aus sein.
3. In HA **Experimentelle Funkübertragung (2 Minuten)** aktivieren.
4. Bei **Gruppe 6** Effekt Dauerlicht und Rot wählen. Bis zu 20 Sekunden warten: Das Armband hört anfangs nur zeitweise zu. Band 1 beobachten.
5. Grün, Blau und anschließend Aus testen. Aus ist ein expliziter Schwarz-Befehl; vor dem Ende des Testfensters mindestens 15 Sekunden senden lassen.
6. Mit **Gruppe 2** an Band 2 wiederholen. Die jeweils andere Gruppe prüfen: Sie soll ihren Sollwert behalten.
7. Ergebnis und Verzögerung in `docs/TESTPROTOKOLL.md` festhalten. Erst dann gilt diese Variante als hardwarebestätigt. Keine zufälligen Rohpakete oder Bruteforce-Sequenzen nötig.

Ein vollständiges Paket dauert nominal 82,4 ms. Es wird höchstens alle 1100 ms begonnen, mit niedriger PA-Einstellung (TI-Nennwert ungefähr −10 dBm bei 433 MHz). ESP-Timer- und WLAN-Jitter sowie reale Ausgangsleistung sind noch mit Messgerät zu prüfen. Sende nur im erlaubten lokalen Frequenzrahmen und mit passender Antenne; diese Software ist keine Funkzulassung. Die Zuteilungen sind bei der [Bundesnetzagentur](https://www.bundesnetzagentur.de/Allgemeinzuteilungen) dokumentiert.

## OTA und Recovery

Normale Folgeupdates erfolgen ohne USB über die lokale Geräte-Webseite. Zuerst Versionshinweise und SHA256SUMS prüfen. Den vorhandenen RESET-Taster kurz drücken: fünf Minuten OTA-Freigabe. Für RGB-Tests kann dieses Fenster nach Anmeldung über **OTA-Fenster schließen und Funk freigeben** vorzeitig geschlossen werden. Auf `http://GERAET/` als `admin` mit dem persönlichen API-Schlüssel anmelden und ausschließlich `bandfunk-VERSION.bin.signed` hochladen. Funk bleibt während OTA gesperrt. Die Signatur wird auf dem ESP vor Aktivierung geprüft; ein fremder oder fehlender Schlüssel wird zurückgewiesen. Neustart folgt nur nach erfolgreichem Abschluss.

Der SHA256-Hash allein ist keine Signatur. Der dauerhaft in der ersten Firmware eingebettete öffentliche Schlüssel ist die Vertrauenswurzel. Privaten Schlüssel offline sichern, niemals in Repository, Pages, Logs oder Releases hochladen. Ein Schlüsselwechsel muss durch eine mit dem alten Schlüssel signierte Firmware erfolgen. Schlüsselverlust erfordert USB-Neuflash. Alte gültig signierte Images können erneut installiert werden: **keine Anti-Rollback-Sperre**.

ESP8266 besitzt hier **keine A/B-Partitionen und keinen automatischen Versionsrollback**. Der Updater schreibt zunächst einen separaten Bereich und aktiviert erst ein vollständig geprüftes Image; Stromausfall beim späteren Bootloader-Kopieren kann trotzdem USB-Recovery erfordern. Einmal USB ist der normale Weg, keine Garantie bei beliebigen Defekten.

- Nach drei Neustarts innerhalb jeweils weniger als 60 Sekunden startet die Firmware ohne Funk im Recovery-Modus und öffnet das Einrichtungsportal. Nach bestätigtem WLAN-Wechsel sind Geräte-Webseite und signiertes OTA wieder verfügbar. Nach einer Minute stabiler Laufzeit wird der Zähler zurückgesetzt.
- Für neue WLAN-Daten den RESET-Taster dreimal mit einigen Sekunden Abstand drücken, bevor eine Minute stabiler Laufzeit vergangen ist: Der Recovery-Modus öffnet das zeitlich begrenzte Portal. Nach Einrichtung mindestens eine Minute laufen lassen und normal neu starten, um den Recovery-Modus zu verlassen. API-Schlüssel bleibt erhalten. Ein beim Start nicht erreichbares Heim-WLAN führt ebenfalls zum zeitlich begrenzten Portal.
- Bei unbootbarer Firmware, kaputtem Dateisystem oder verlorenem Schlüssel: USB-Web-Flasher oder `pio run -t upload`; Flash-Löschen entfernt auch WLAN/API-Daten. GPIO0 nur für manuellen Bootloaderstart nach Herstelleranleitung auf LOW halten. D8 LOW, D4 HIGH; Funkverdrahtung nötigenfalls abnehmen.
- Die integrierten WiFiManager-Routen `/update`, `/u`, `/erase` und weitere administrative Nebenwege sind über vorangestellte Sperrhandler deaktiviert. Formularspeichern erfordert einen zufälligen Schlüssel und den Zugriff über die AP-Schnittstelle. Kein automatisches Formatieren bei Dateisystemfehlern. Weder ArduinoOTA noch ein unsignierter OTA-Nebenweg ist aktiviert.

## Selbst bauen und signieren

Python 3.12+ für die Buildwerkzeuge, für HA-Integrationstests CI mit Python 3.13. PlatformIO-Plattform und ArduinoJson sind festgelegt. Vorhandenen OTA-Schlüssel weiterverwenden.

```sh
python -m pip install -r requirements-build.txt
# Nur für eine eigene, neue Vertrauenswurzel; niemals vor jedem Release!
python tools/keys.py --private /sicherer/ort/ota-private.pem
python -m platformio run --project-dir firmware
python -m pytest tests -q
python tools/release.py --private /sicherer/ort/ota-private.pem
```

Das Repository enthält bereits den öffentlichen Projektschlüssel: `keys/ota-public.pem`. Für einen eigenen Fork diesen bewusst durch den passenden eigenen öffentlichen Schlüssel ersetzen und einmalig USB flashen. `keys.py` überschreibt bestehende Schlüssel absichtlich nicht.

`dist/` enthält Factory-BIN, signiertes OTA-BIN, HACS-ZIP mit Dateien an der ZIP-Wurzel, Web-Tools-Manifest und SHA256SUMS. ZIP-Zeitstempel und Reihenfolge sind deterministisch. RSA-PKCS1v1.5/SHA256 ist für dasselbe Image und denselben Schlüssel deterministisch. Bitidentische Firmware zusätzlich mit identischem Buildpfad, Toolchain, Abhängigkeiten und Umgebung vergleichen; keine ungeprüfte Zusage plattformübergreifend identischer Binärdateien. `docs/RELEASE.md` beschreibt CI, Veröffentlichung und Offline-Signierung.

## Diagnose und Grenzen

HA bietet WLAN-RSSI, Laufzeit, freien RAM, Firmware, Flanken-/Verlustzähler, Funkmodul-, Recovery- und OTA-Status. Diagnoseexport benutzt eine feste Freigabeliste ohne API-Schlüssel, IP, MAC, SSID oder WLAN-Passwort. Der API-Schlüssel wird in HA wie andere Integrationseinstellungen gespeichert: HA-Backups schützen.

Der Empfangsring ist auf 255 Flanken begrenzt und wird im Hauptprogramm geleert. Die V0.1.7 zählt Flanken, ist aber kein vollständiger Mitschnitt- oder OOK-Decoder. Für genaue Zeitmessung Logic Analyzer oder SDR verwenden. `ProtocolDecoder` bildet den Einstieg für spätere Empfängerdecoder; `crowdsync.h` kapselt Paket- und Wellenformerzeugung des experimentellen Sendetreibers. Es gibt keinen frei zugänglichen Rohsender-Endpunkt.

Die Funkstrecke ist unidirektional: keine tatsächliche Farbrückmeldung. Gruppen-Schalterzustand bedeutet Sollwert, Funkfreigabe bedeutet Testfenster. Random-/Blinkeffekte und Energiesparen hängen vom Armbandmodell ab. Nach dem Test immer Schwarz senden; bloßes Senderabschalten garantiert keine dunklen LEDs.
