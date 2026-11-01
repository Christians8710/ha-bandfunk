# Fremdkomponenten und Zuordnung

Die eigenen Projektdateien stehen unter MIT. Diese Lizenz ersetzt nicht die Lizenzen der Abhängigkeiten.

- Protokollreferenz: James Wang, [CrowdSync_RF](https://github.com/jamesw343/CrowdSync_RF), Commit `b800899687224011657e4ac62caaf45d54892f6d`, MIT. Paketformat und nominale Zeiten sind im eigenen Encoder umgesetzt. Lizenztext: `licenses/CrowdSync_RF-MIT.txt`. Keine MCU-Dumps, Hersteller-Firmware oder Bilder übernommen.
- ESP8266 Arduino Core 3.1.2: LGPL-2.1-or-later und komponentenspezifische Lizenzen, einschließlich Espressif SDK und BearSSL. Quellcode und Lizenzdateien über [esp8266/Arduino](https://github.com/esp8266/Arduino/tree/3.1.2); Buildrezept in `firmware/platformio.ini`. Für eine Weiterverteilung die jeweiligen Hinweise/Lizenzpflichten beachten; eigener MIT-Code ändert daran nichts.
- ArduinoJson 6.21.5: MIT, [Quellcode und Lizenz](https://github.com/bblanchon/ArduinoJson/tree/v6.21.5).
- ESP Web Tools 10.1.0: Apache-2.0, [Projekt](https://github.com/esphome/esp-web-tools). Das Webmodul wird aus dem festgelegten CDN-Pfad geladen und ist kein selbst geschriebener Bestandteil.
- Home Assistant Core: Apache-2.0. Keine Home-Assistant-Marke oder Logos in eigene Assets übernommen.
- PlatformIO, pytest, cryptography und aiohttp sind Build-/Testwerkzeuge bzw. von HA bereitgestellte Laufzeitkomponenten; sie werden nicht als Python-Pakete mit dem HACS-ZIP ausgeliefert.

BandFunk ist eine unabhängige Projektbezeichnung. Die kurze Namens-Websuche ist keine vollständige Markenfreigabe. CrowdSync, Wemos/LOLIN und weitere Produktnamen werden ausschließlich zur Beschreibung der untersuchten Hardware verwendet.
