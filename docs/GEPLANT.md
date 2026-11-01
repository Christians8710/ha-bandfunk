# Nächste Ausbauschritte

## Mehrere Gruppen zusammen steuern

Vorgemerkt: Auf der Geräte-Webseite sollen mehrere Gruppen auswählbar sein. Eine Änderung von Farbe, Helligkeit, Ein/Aus oder Effekt soll dann für alle ausgewählten Gruppen gleichzeitig gelten. Nicht ausgewählte Gruppen sollen ihre Werte behalten. Noch nicht umgesetzt.

## Kleine Szenarien

Gewünscht und bestätigt: beides ergänzen – gespeicherte Szenen mit Farbeinstellungen für mehrere Gruppen und zeitliche Abläufe, zum Beispiel Rot → Blau → Aus. Noch nicht umgesetzt.

Konkretes Beispiel: Blaulicht mit abwechselnd blinkenden Gruppen. Zuerst Gruppe 2 blau und Gruppe 6 aus, danach Gruppe 2 aus und Gruppe 6 blau; diesen Wechsel wiederholen. Die Gruppen sollen auswählbar sein. Dafür sind Start/Stopp und ein einstellbares Tempo vorgesehen. Die tatsächlich mögliche Geschwindigkeit muss am Funkcontroller und an den Armbändern geprüft werden; die beiden LEDs innerhalb eines einzelnen Bandes sind damit nicht getrennt adressiert.

Die eigenständige Bedienung ohne Home Assistant bleibt dabei die Grundlage. Eine spätere Umsetzung soll auf dem ESP laufen; Home Assistant bleibt eine optionale Ergänzung. Bedienung, Zeiten, Wiederholung, Start/Stop und Verhalten bei Neustart werden vor der Umsetzung konkret festgelegt.
