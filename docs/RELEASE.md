# Release-Ablauf

1. Tests und Firmware-Build müssen grün sein. Hardwarestatus in den Versionshinweisen offen nennen.
2. Öffentlichen OTA-Schlüssel mit der bereits installierten Firmware vergleichen. Privaten Schlüssel niemals neu generieren, um einen fehlenden Secret zu umgehen.
3. Lokal mit `tools/release.py` signieren. Tool prüft die erzeugte Signatur gegen den eingecheckten öffentlichen Schlüssel, bevor es die Datei ausliefert.
4. Factory-BIN bei Offset 0 dient nur USB. OTA nutzt ausschließlich die Datei mit `.bin.signed`. Kein Dateisystem-Image über OTA verteilen.
5. Release `v0.1.0` mit HACS-ZIP, beiden BIN-Dateien, Manifest und SHA256SUMS anlegen. Für experimentelle Hardware den Status im Release-Titel und Text nennen.
6. Pages-Workflow manuell starten, nachdem die geprüften Factory-Dateien unter `web/` liegen. GitHub Pages muss als Quelle GitHub Actions gewählt sein. Die Website benötigt HTTPS; keine direkte Verbindung von HTTPS-Pages zur HTTP-API des Geräts.

CI baut die Firmware mit dem **öffentlichen** Schlüssel und führt Python- sowie native Paket-/Timingtests aus. CI erzeugt keine vertrauenswürdige OTA-Signatur ohne privaten Schlüssel. Ein manueller Release-Workflow erzeugt einen Draft aus dem CI-Build; signierte Dateien werden offline ergänzt und erst danach veröffentlicht. Keine privaten Schlüssel als Workflow-Artefakte.

Für spätere Releases die Versionskonstante in Firmware, manifest.json, Release-Tool, Flasher und Dokumentation gemeinsam ändern. Vorhandene Schlüssel und LittleFS-Layout erhalten. Alte signierte Images können als manueller Recovery-Downgrade dienen, solange ihr Layout kompatibel bleibt.

Signaturformat: Roh-Firmware + 256-Byte RSA-PKCS1v1.5/SHA256-Signatur + 32-Bit Little-Endian Signaturlänge. Kompatibel zum ESP8266-Arduino-Updater 3.1.2. Prüfsummen sind zusätzliche Transportkontrolle.
