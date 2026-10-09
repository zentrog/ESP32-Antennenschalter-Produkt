# ESP32-Antennensteuerung

Eine browserbasierte Steuerung für ESP32, Relais, Funkgeräte und Antennen. Ein einzelner ESP kann allein arbeiten. Mehrere ESPs können optional als Master und Slaves zusammenarbeiten.

Aktueller Produktkandidat: Firmware 1.8.5.

Die gespeicherte Anordnung der Anlagenteile bleibt auf Handy, Laptop und großem Monitor an denselben Rasterpositionen. Die Ansicht verkleinert Raster und Beschriftungen an die verfügbare Fläche; sie ordnet die Geräte nicht automatisch um.

Die Release-Datei `firmware-esp32dev.bin` enthält die vollständige Firmware und Weboberfläche für ein manuelles OTA-Update. Die getrennte LittleFS-Partition mit WLAN und Gerätekonfiguration wird dabei nicht überschrieben.

## Für Anwender

Die Schritt-für-Schritt-Anleitung steht in [ANLEITUNG.md](ANLEITUNG.md). Sie erklärt Installation, WLAN-Einrichtung, Relais, Signalwege, Verbund und Updates ohne vorausgesetztes Fachwissen.

## Leere Produktversion

Eine neue Installation enthält absichtlich keine persönlichen oder standortspezifischen Daten:

- keine WLAN-Namen oder Kennwörter
- kein Rufzeichen und keine Postleitzahl
- keine Relais, Schaltfunktionen oder logischen Geräte
- keine Anlagenwege und keine Gerätegruppen
- keine Beispiel- oder Testanlage

Ein normales Firmware-Update löscht keine vorhandenen Geräteeinstellungen. Vor einem normalen OTA-Update sichert der ESP seine aktuelle Konfiguration. Bei einem Dateisystemfehler wird nicht automatisch formatiert.

## Bauen

Benötigt werden Visual Studio Code und PlatformIO IDE. Für das übliche ESP32-DevKit mit 30 Pins wird nur das PlatformIO-Ziel **esp32dev** verwendet.

Im PlatformIO-Terminal:

    pio run -e esp32dev
Beim ersten Flashen eines neuen Gerätes muss zusätzlich das LittleFS-Dateisystem gebaut und geladen werden. Das überschreibt den Gerätespeicher und gehört nicht zu einem normalen Firmware-Update. Die vollständigen Schritte stehen in [ANLEITUNG.md](ANLEITUNG.md).

## Updates

Die Weboberfläche prüft GitHub Releases auf die neueste stabile Version. Die Schaltfläche **Neueste Firmware direkt herunterladen** lädt die einzige vollständige Firmware-Datei direkt herunter, ohne die Release-Seite zu öffnen oder ein Asset auszuwählen. Danach startest du das manuelle OTA am Gerät.

Eine Sicherungsdatei enthält die lokalen Einstellungen des geöffneten ESP, die gemeinsame Anlagenkonfiguration und dessen gespeicherte WLANs. Im Verbund sammelt der Master die lokalen Einstellungen der Slaves nicht ein: Öffne deshalb jeden ESP über seine eigene Adresse und lade dort eine eigene Sicherungsdatei herunter. Controller-ID und IP stehen im Dateinamen und in der Datei.

## Projektunterlagen

- [Anleitung für Anwender](ANLEITUNG.md)
- [Architektur](docs/ARCHITECTURE.md)
- [HTTP-API](docs/API.md)
- [Prüfprotokoll](docs/VALIDATION.md)
- [Dokumentationsablauf](docs/DOCUMENTATION-WORKFLOW.md)

## Datenschutz

WLAN-Kennwörter, Rufzeichen, Postleitzahlen, Anlagenkonfigurationen und Gerätekennungen gehören in den ESP und nicht in öffentliche Quelltexte, Screenshots oder GitHub-Protokolle. Vor einer Veröffentlichung müssen Dateien und die erreichbare Git-Historie geprüft sein.
