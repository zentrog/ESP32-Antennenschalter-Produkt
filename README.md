# ESP32-Antennensteuerung

Eine browserbasierte Steuerung für ESP32, Relais, Funkgeräte und Antennen. Ein einzelner ESP kann allein arbeiten. Mehrere ESPs können optional als Master und Slaves zusammenarbeiten.

Aktueller Produktkandidat: Firmware 1.8.0.

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

Benötigt werden Visual Studio Code und PlatformIO IDE. Das Standardziel für ein übliches ESP32-DevKit mit 30 Pins ist **esp32dev**; **esp32dev-jungfrau** baut dieselbe leere Startkonfiguration.

Im PlatformIO-Terminal:

    pio run -e esp32dev
    pio run -e esp32dev-jungfrau

Beim ersten Flashen eines neuen Gerätes muss zusätzlich das LittleFS-Dateisystem gebaut und geladen werden. Das überschreibt den Gerätespeicher und gehört nicht zu einem normalen Firmware-Update. Die vollständigen Schritte stehen in [ANLEITUNG.md](ANLEITUNG.md).

## Updates

Die Weboberfläche prüft GitHub Releases auf die neueste stabile Version. Die Prüfung zeigt nur Informationen; die Installation wird bewusst vom Benutzer gestartet. Release-Dateien und Änderungen stehen nach Einrichtung des öffentlichen Produkt-Repositories auf der [GitHub-Release-Seite](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/latest).

## Projektunterlagen

- [Anleitung für Anwender](ANLEITUNG.md)
- [Architektur](docs/ARCHITECTURE.md)
- [HTTP-API](docs/API.md)
- [Prüfprotokoll](docs/VALIDATION.md)
- [Dokumentationsablauf](docs/DOCUMENTATION-WORKFLOW.md)

## Datenschutz

WLAN-Kennwörter, Rufzeichen, Postleitzahlen, Anlagenkonfigurationen und Gerätekennungen gehören in den ESP und nicht in öffentliche Quelltexte, Screenshots oder GitHub-Protokolle. Vor einer Veröffentlichung müssen Dateien und die erreichbare Git-Historie geprüft sein.
