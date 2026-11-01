# Änderungen

## 0.2.0

BandFunk wird eigenständiger Web-Funkcontroller; Home Assistant bleibt optional. Lokale deutsche Webseite mit Logo, Gruppenwahl, Ein/Aus, Farbe, Helligkeit, Effekten und Dauerfreigabe. Funkfreigabe und zehn Gruppeneinstellungen bleiben nach Neustart erhalten; AUS bewahrt die zuletzt gewählte Farbe. Atomare Konfigurationsspeicherung nach drei Sekunden Ruhe statt Flash-Schreiben bei jedem Slider-Schritt.

Separater Webzugang: Benutzer `admin`, Startpasswort `bandfunk2026`, auf der Webseite änderbar. Passwort-Digest statt Klartext gespeichert. Browser-Sitzungen, Abmeldung und CSRF-Schutz für Änderungen; bisheriger HA-API-Schlüssel und WLAN-Daten werden übernommen. Vergessenes Webpasswort lässt sich ausschließlich im physisch gestarteten Einrichtungs-WLAN zurücksetzen. Keine Ausgabe des API-Schlüssels mehr im seriellen Startprotokoll; Anzeige nach Web-Anmeldung bleibt.

Signiertes OTA weiterhin nur im fünfminütigen Fenster nach physischem RESET. Das Berechtigungsfenster blockiert den normalen Funkbetrieb nicht mehr; während des Uploads werden Timer und Funk gestoppt. Abgewiesene oder abgebrochene Uploads geben den Funk anschließend wieder frei. Kein A/B-Rollback; USB-Recovery bleibt für defekte Startcodes nötig.

HA erhält gespeicherte Farbe/Helligkeit/Ein-Aus separat und gleicht Änderungen der Weboberfläche alle fünf Sekunden ab. Bestehende Entitäts-IDs bleiben erhalten; ältere API-1-Firmware bleibt unterstützt. API-1-Bearer-Endpunkte um typgeprüfte Steuerdaten erweitert; browserbasierte Administration kann nicht mit dem HA-API-Schlüssel übernommen werden.

Hardwarestand: Besitzer bestätigt, dass beide Bänder mit aufgedruckter Gruppe 2 und 6 über HA reagieren. Das ist keine Bestätigung aller Effekte oder aller V5-Geräte.

## 0.1.8 (HA-Integration)

Lokales BandFunk-Symbol für Home Assistant ergänzt: `brand/icon.png` (256 px) und `brand/icon@2x.png` (512 px), transparent für helle und dunkle Oberflächen. Seit Home Assistant 2026.3 wird es direkt aus der installierten Custom-Integration geladen; kein externer Brands-PR erforderlich. Motiv aus dem freigegebenen BandFunk-Logo abgeleitet. Firmware bleibt 0.1.7; kein ESP-Update nötig.

## 0.1.7

Bei einer Registerabweichung zeigt die Funkdiagnose die genaue CC1101-Adresse sowie Soll- und Istwert. 0.1.6 startete nach dem signierten OTA erfolgreich, meldete im Test aber eine Registerabweichung vor dem ersten Sendestart. Diese wird jetzt einzeln auswertbar. Noch keine bestätigte Funksteuerung an V5.

## 0.1.6

Korrigiert die Diagnose-Erweiterung: Der vergrößerte Status-JSON-Puffer liegt auf dem Heap statt dem begrenzten ESP8266-Stack. 0.1.5 zeigte beim Gerätetest Abstürze nach dem WLAN-Start und darf nicht für neue Installationen verwendet werden. 0.1.6 ersetzt diese Diagnoseversion.

## 0.1.5

Funkdiagnose: kritische CC1101-Register und OOK-Leistungstabelle werden vor jedem Paket zurückgelesen. Senden startet erst nach bestätigtem TX-Zustand und geprüfter PLL-Kalibrierung statt nach einer festen 800-µs-Wartezeit. SPI-Fehler, ausbleibender Sendestatus und abgebrochene Impulsfolgen stoppen den Test. Zähler unterscheiden Versuche, Chip-TX-Starts und vollständig ausgegebene Impulsfolgen. Geräte-Webseite und HA-Diagnosedownload zeigen die Messwerte. Timer-Zeitlimit und interne Impulsverspätungsstatistik ergänzt; native Fehlerpfadtests.

Das ist keine Empfangsbestätigung und kein Nachweis der abgestrahlten Leistung. V5-Armbänder bleiben unbestätigt; der Rot-Test mit 0.1.4 blieb auch nach Batterie-Neueinsetzen ohne Reaktion. Das Wristbanditø-Dokument zeigt ebenfalls V5-Hardware, enthält aber keinen entschlüsselten Funkbefehl.

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
