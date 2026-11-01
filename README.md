# BandFunk · V0.2.1

![BandFunk](web/assets/bandfunk-logo-v1.png)

**Eigenständiger, deutschsprachiger Web-Funkcontroller für LED-Armbänder. Home Assistant ist eine optionale Ergänzung.** Der Wemos D1 mini mit CC1101 arbeitet lokal, ohne Cloud und ohne laufende Verbindung zu HA. Auf seiner Webseite lassen sich Gruppen, Ein/Aus, Farbe, Helligkeit und Effekte einstellen. Die Funkfreigabe bleibt dauerhaft aktiv, bis sie ausgeschaltet wird.

## Hardwarestand

Meine beiden Armbänder tragen die Platinenkennung `23A033V5`. Auf dem einen steht die **6**, auf dem anderen die **2**. Beide reagieren auf die Steuerung über Home Assistant. Ich habe noch nicht jeden Effekt einzeln getestet und kann bisher nur für meinen Aufbau sprechen. Die Anzeige in der Webseite und in HA zeigt den eingestellten Wert – eine Rückmeldung vom Armband gibt es nicht.

Paketformat und Zeiten sind aus der MIT-lizenzierten, unabhängig untersuchten [CrowdSync_RF-Referenz](https://github.com/jamesw343/CrowdSync_RF/tree/b800899687224011657e4ac62caaf45d54892f6d) umgesetzt. Diese Referenz betrifft eine V7_2-Platine. Keine Hersteller-Firmware oder fremden Platinenfotos werden verteilt. BandFunk ist ein unabhängiges Projekt ohne Verbindung zum CrowdSync-Hersteller oder zum Home-Assistant-Projekt.

## Anschluss

Wemos/LOLIN D1 mini mit **4 MB**, EBYTE E07-M1101D bzw. passende SMA-Variante. Nur **3,3 V**, gemeinsame Masse, kurze Leitungen, passende 433-MHz-Antenne, 100 nF und 10 µF am Modul. Vor Umstecken USB abziehen.

| CC1101-Modul-Pin | Signal | D1 mini |
|---|---|---|
| 1 | GND | G / GND |
| 2 | VCC | 3V3 |
| 3 | GDO0 | D2 / GPIO4 |
| 4 | CSN | D1 / GPIO5, 10-kΩ-Pull-up nach 3V3 empfohlen |
| 5 | SCK | D5 / GPIO14 |
| 6 | MOSI | D7 / GPIO13 |
| 7 | MISO / GDO1 | D6 / GPIO12 |
| 8 | GDO2 | frei |

**D8 bleibt frei.** Die alte Empfehlung CSN an D8 ist überholt und passt nicht zu dieser Firmware: GPIO15 muss beim Boot LOW sein. Auch D0, D3 und D4 bleiben beim Standardaufbau frei. Der vorhandene RESET-Taster ist für OTA/Recovery vorgesehen. D0 ist nur mit externem Pull-up und explizitem Buildflag als Zusatztaster verwendbar.

Pinnummern und Modellansicht mit den [EBYTE-Unterlagen](https://www.ebyte.com/product/58.html) abgleichen; die Tabelle ist keine universelle geometrische Ansicht aller CC1101-Boards. [LOLIN D1 mini](https://www.wemos.cc/en/latest/d1/d1_mini.html), [TI CC1101](https://www.ti.com/lit/ds/symlink/cc1101.pdf).

## Einmalig installieren

1. [HTTPS-Web-Flasher](https://christians8710.github.io/ha-bandfunk/) auf dem PC mit dem angeschlossenen ESP in Desktop-Chrome oder Edge öffnen. Der COM-Port gehört zu diesem PC, nicht zu einem HA-Server.
2. Version **0.2.1** per USB installieren. Ein vollständiges Löschen entfernt vorhandene WLAN-/API-Daten; bei bereits eingerichtetem Gerät stattdessen signiertes OTA verwenden.
3. Mit dem Einrichtungs-WLAN `bandfunk-…` verbinden, WLAN-Passwort **bandfunk2026**. In Chrome `http://192.168.4.1` öffnen.
4. Heim-WLAN aus der Liste auswählen und dessen Passwort eingeben. WiFiManager 2.0.17 prüft die Verbindung und zeigt bei Erfolg die neue IP. Bis zu 30 Sekunden warten. Bei Fehlern bleibt das Portal für einen weiteren Versuch offen. Nach Erfolg ist der AP noch 30 Sekunden sichtbar; insgesamt maximal zehn Minuten.
5. Wieder mit dem Heimnetz verbinden und die angezeigte IP oder `http://bandfunk-GERAETEKENNUNG.local/` öffnen. Anmeldung: **admin / bandfunk2026**. Danach kann das Admin-Passwort auf der Webseite geändert werden.

Die Geräteoberfläche samt Logo funktioniert lokal ohne CDN oder Home Assistant. Der HA-API-Schlüssel steht geschützt nach Anmeldung auf der Geräte-Webseite; er ist unabhängig von beiden Passwörtern.

## Direkt im Browser steuern

- Funkübertragung dauerhaft aktivieren oder ausschalten. Standard bei Erstinstallation bzw. Migration: aktiv. Nach einem Funkfehler wird die Freigabe abgeschaltet.
- Die auf dem Band aufgedruckte Gruppe wählen. Alle Bänder derselben Gruppe erhalten dieselben Werte; keine individuelle Adressierung.
- Ein/Aus, Farbe, Helligkeit und einen der 16 Effekte wählen. AUS überträgt Schwarz und bewahrt die letzte Farbe für das nächste Einschalten.
- Einstellungen werden nach drei Sekunden ohne weitere Änderung atomar gespeichert und nach Neustart wiederhergestellt. Die Oberfläche zeigt ausstehende Speicherung bzw. Speicherfehler an.
- Bei ausgeschaltetem Hauptschalter werden Sollwerte weiter gespeichert, aber nicht gefunkt. Ausschalten der Funkfreigabe ist kein LED-Aus-Befehl: zuerst die gewünschte Gruppe ausschalten und die Übertragung einige Sekunden laufen lassen.

Die Funkfreigabe hat **kein Zwei-Minuten-Limit** mehr. Wiederholte Pakete starten weiterhin höchstens alle 1100 ms, bei niedriger PA-Einstellung (TI-Nennwert ungefähr −10 dBm bei 433 MHz). Es entsteht kein ununterbrochener Funkträger. Ein Paket dauert nominal 82,4 ms; die zuletzt gemessene ESP-Impulsfolge lag bei etwa 84,3 ms. Reale Leistung/Spektrum sind noch nicht extern gemessen. Nur im örtlich zulässigen Funkrahmen einsetzen.

Ab 0.2.1 beginnt 1 % bei der nutzbaren Mindesthelligkeit (ungefähr dem bisherigen 5-%-Bereich); 0 % ist aus. Die 16 physisch möglichen Stufen je Farbkanal werden dadurch nicht feiner. Browser und HA verwenden dieselbe Umrechnung. Schnelle Änderungen werden geordnet; nach einem verlorenen Browser-Aufruf wird die Bedienung spätestens nach sechs Sekunden wieder frei.

Copyright und Lizenz stehen direkt auf der Geräte-Webseite unter **MIT-Lizenz und Drittanbieter**. Die vollständigen Texte sind lokal auf `/license` erreichbar, auch ohne Internet.

## Home Assistant optional ergänzen

1. In HACS das benutzerdefinierte Repository `https://github.com/Christians8710/ha-bandfunk` als **Integration** hinzufügen und BandFunk herunterladen, anschließend HA neu starten.
2. Einstellungen → Geräte & Dienste → **BandFunk HA** hinzufügen. mDNS-Discovery funktioniert im gemeinsamen Multicast-Netz; andernfalls Geräte-IP manuell eingeben.
3. Den **HA-API-Schlüssel** von der Geräte-Webseite eintragen. Nicht das Admin- oder WLAN-Passwort verwenden.

Webseite und HA verwenden dieselben zehn Gruppeneinstellungen und denselben Hauptschalter. HA aktualisiert Webänderungen spätestens nach etwa fünf Sekunden. Bestehende Entitäts-IDs und API-Schlüssel bleiben beim Update erhalten. Die bisherigen API-1-Firmwareversionen bleiben unterstützt. Lokale HA-Logos benötigen HA 2026.3 oder neuer; die übrigen Funktionen benötigen mindestens HA 2025.1.

## Passwörter und Zugang

| Zweck | Zugang |
|---|---|
| Einrichtungs-WLAN | festes WLAN-Passwort `bandfunk2026` |
| Geräte-Webseite | Benutzer `admin`, anfangs `bandfunk2026`, später selbst ändern |
| HA-Integration | persönlicher, gerätespezifischer API-Schlüssel |

Der Web-Passwort-Digest wird lokal gespeichert, nicht das Klartext-Adminpasswort. Browser-Sitzungen laufen nach 24 Stunden oder Geräte-Neustart ab. Passwortänderung meldet sämtliche Browser-Sitzungen ab. Änderungen erfordern Anmeldung und einen Seitenschlüssel gegen fremde Formularaufrufe. Die lokale HTTP-Verbindung ist nicht Ende-zu-Ende verschlüsselt: nur im vertrauenswürdigen LAN betreiben, nicht ins Internet freigeben. HA-Backups enthalten den API-Schlüssel und müssen entsprechend geschützt werden.

Bei vergessenem Webpasswort dreimal mit einigen Sekunden Abstand RESET drücken, bevor eine Minute stabiler Betrieb vergangen ist. Im physisch gestarteten Einrichtungs-WLAN auf `192.168.4.1` lässt sich nur das Adminpasswort auf `bandfunk2026` zurücksetzen. WLAN-Daten und HA-Schlüssel bleiben erhalten. Nach mindestens einer Minute stabiler Laufzeit normal neu starten, um Recovery zu verlassen.

## Signierte OTA-Updates und Recovery

Den RESET-Taster einmal kurz drücken: signierte Firmware darf für fünf Minuten installiert werden. Auf der Geräte-Webseite anmelden, **Firmware aktualisieren** öffnen und `bandfunk-VERSION.bin.signed` hochladen. Der normale Funkbetrieb bleibt während des Berechtigungsfensters möglich; Timer und Funk werden erst während des eigentlichen Uploads gestoppt. Ungültige/abgebrochene Uploads aktivieren keine neue Firmware und geben den Funk wieder frei. Bei Erfolg folgt ein Neustart, die Anmeldung ist danach erneut erforderlich.

Es gelten weiterhin **RSA-2048/SHA-256**, unveränderter öffentlicher Vertrauensschlüssel, zusätzliche physische Freigabe und Anmeldung. Weder Bibliotheks-OTA noch ArduinoOTA oder ein unsignierter Nebenweg sind aktiv. Private Schlüssel werden nicht auf GitHub oder Pages hochgeladen. Schlüsselverlust bzw. kaputter Bootcode kann USB-Recovery erfordern. ESP8266 hat hier kein A/B-System und keinen automatischen Rollback; Stromausfall beim Bootloader-Kopieren kann USB erfordern. Alte gültige signierte Images werden nicht per Anti-Rollback gesperrt. Downgrade auf Firmware vor 0.2.0 kann neue Einstellungen verlieren und wieder den HA-API-Schlüssel als Webpasswort verwenden.

## Build, Tests und Artefakte

```sh
python -m pip install -r requirements-build.txt
python -m platformio run --project-dir firmware
python tools/check_stack.py firmware/.pio/build/d1_mini/src/main.cpp.su
python -m pytest tests -q
python tools/release.py --private /sicherer/ort/ota-private.pem
```

Vorhandenen OTA-Schlüssel weiterverwenden. Plattform, ArduinoJson und WiFiManager sind festgelegt; das Logo wird beim Build lokal eingebettet. Native Tests prüfen Paketformat, Zeiten, Register-Fehlerpfade, Farb-/Helligkeitsumrechnung und Passwortgrenzen. Python/Node führen die tatsächliche Web-Steuerlogik aus; CI testet zusätzlich echte HA-Klassen, hassfest, HACS und den ESP-Stackrahmen. Signaturtests prüfen passende/falsche Schlüssel und manipulierte Images.

`dist/`: HACS-ZIP, Factory-BIN, signiertes OTA-BIN, Manifest und Prüfsummen. ZIP und Signatur sind für gleiche Eingaben deterministisch. Plattformübergreifend bitgleiche Firmware wird nicht ungeprüft versprochen. [Release-Ablauf](docs/RELEASE.md), [Gerätetests und Grenzen](docs/TESTPROTOKOLL.md), [Lizenzen](THIRD_PARTY_NOTICES.md).
