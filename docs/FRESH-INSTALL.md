# Leere Produktinstallation prüfen

Der einzige PlatformIO-Build esp32dev startet mit einer leeren Produktkonfiguration. Er enthält keine WLAN-Zugangsdaten oder Beispielgeräte.

## Erwarteter Erststart nach Löschen eines neuen Testgeräts

- Kein gespeichertes WLAN; das Gerät zeigt sein Einrichtungs-WLAN.
- Rufzeichen, Postleitzahl, Beschreibung und Standort sind leer.
- Es sind keine Relais, Schaltfunktionen, logischen Geräte, Routen oder Layout-Beispiele angelegt.
- Die sieben öffentlichen NewsTicker-Standardquellen sind vorhanden; nur n-tv ist standardmäßig aktiviert. Sie enthalten keine privaten Anlagendaten.
- Die Verbundrolle ist unassigned; eine Systemkennung oder ein Systemname ist nicht vorgegeben.
- Der Anwender trägt WLAN, Platine, Relais und Anlagenteile selbst ein.

Technische Werte wie Controller-ID, neutraler Gerätename, Firmwareversion und benötigte NTP-Server sind keine persönlichen Anlagendaten.

## Sicherer Testablauf

1. Verwende einen neuen oder ausdrücklich entbehrlichen ESP. Ein vollständiges Löschen entfernt Gerätespeicher und gespeicherte Konfigurationen.
2. Lade esp32dev als Firmware und bei einem neuen Gerät einmalig das LittleFS-Dateisystem.
3. Verbinde dich mit dem angezeigten Einrichtungs-WLAN und öffne die Einrichtungsseite.
4. Lies die Grunddaten und Konfigurationslisten aus. Prüfe, dass Identität und Listen leer sind.
5. Notiere nur Version, Buildumgebung und die Anzahl der Einträge. Keine Kennwörter, Rufzeichen oder privaten Gerätekonfigurationen in ein öffentliches Prüfprotokoll schreiben.

Der Test verändert oder löscht keine Konfiguration der bereits eingerichteten Anlagen. Ergebnisse nur nach tatsächlicher Ausführung in docs/VALIDATION.md eintragen.

## Manuelle Wiederherstellung einer Gerätesicherung

Nach einem vollständigen Flash-Löschen erscheint das offene Einrichtungs-WLAN `AntennaController-XXXXXX`. Zuerst über `http://192.168.4.1` ein WLAN einrichten, damit die Hauptoberfläche im normalen Netzwerk erreichbar wird. Dann dort **Konfigurieren → Sicherheit → Sicherung / Wiederherstellung → Konfiguration importieren** öffnen, die zum Controller passende Sicherungsdatei auswählen und den Import bestätigen. Die JSON-Sicherung enthält lokale Konfiguration einschließlich Verbundzuordnung, gemeinsame Konfiguration und WLAN-Daten. Für den vollständigen Import muss Firmware 1.8.13 oder neuer installiert sein.
