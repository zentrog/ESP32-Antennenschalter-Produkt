# ESP32-Antennensteuerung

Eine browserbasierte Steuerung für ESP32, Relais, Funkgeräte und Antennen. Ein einzelner ESP kann allein arbeiten. Mehrere ESPs können optional als Master und Slaves zusammenarbeiten.

Aktuelle veröffentlichte Produktversion: Firmware 1.8.8. [Release und Direktdownload](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/latest).

Unter **Diagnose** gibt es ein freiwilliges Fehler-/Wunschformular mit sicherer Vorschau und lokalem Download-Fallback. Der HTTPS-Mail-Endpunkt auf do1anb.de und der SMTP-Versand wurden am 10.10.2026 live geprüft. Der Bericht wird erst nach ausdrücklicher Bestätigung versendet; Details und verbleibende Prüfungen stehen unter [Melde-Endpunkt und Freigabestatus](docs/REPORT-ENDPOINT.md).

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
- [Fehler- und Wunschmeldungen](docs/REPORT-ENDPOINT.md)
- [Nutzung und Sicherheitshinweise](docs/LEGAL-NOTICES.md)
- [Drittanbieter-Lizenzen](THIRD-PARTY-NOTICES.md)
- [Geplante Windows-App: Funktionsumfang und Umsetzung](docs/WINDOWS-APP-PLAN.md) (geplant, noch nicht implementiert)

Die Windows-App ist optional und noch nicht implementiert. Firmware bleibt separat herunterladbar, manuell installierbar und eigenständig gepflegt. Die App wird später einen zusätzlichen geführten Weg bieten.

## Projektkontakt und Urheberangabe

© 2026 Andreas Bodyn (DO1ANB) · Projektkontakt: [andreas.bodyn@gmx.de](mailto:andreas.bodyn@gmx.de)

Freiwillige Unterstützung: [PayPal.Me](https://paypal.me/andreasbodyn). Das Projekt bleibt frei verfügbar; Unterstützung ist freiwillig und begründet keinen Anspruch auf Support.

## Lizenz

Dieses Projekt steht unter der GNU General Public License, Version 3 oder (nach deiner Wahl) jeder späteren Version. Siehe [LICENSE](LICENSE). Bibliotheken und Werkzeuge Dritter behalten ihre jeweils eigenen Lizenzen.

GPL erlaubt auch kommerzielle Nutzung und Weitergabe, wenn die Lizenzbedingungen eingehalten werden. Für eine andere Lizenzierung oder bezahlte Unterstützung kannst du den Projektkontakt ansprechen. Details zu eingebundenen Bibliotheken und den technischen Grenzen stehen in [Drittanbieter-Hinweisen](THIRD-PARTY-NOTICES.md) und [Nutzungshinweisen](docs/LEGAL-NOTICES.md).

Die Firmware enthält keine KI-Funktion und übermittelt keine Anlagenkonfiguration an einen KI-Dienst. Die freiwillige Fehler-/Wunschmeldung übermittelt ausschließlich die vom Nutzer geprüften Formularangaben und die vorher angezeigte Firmware-Version an den Projekt-Mailserver.

## Datenschutz

WLAN-Kennwörter, private Postleitzahlen, Anlagenkonfigurationen und Gerätekennungen gehören in den ESP und nicht in öffentliche Quelltexte, Screenshots oder GitHub-Protokolle. Der Projektkontakt und das Rufzeichen DO1ANB sind vom Betreiber ausdrücklich zur öffentlichen Projektangabe freigegeben. Vor jeder Veröffentlichung müssen Dateien und die erreichbare Git-Historie auf nicht freigegebene private Daten geprüft sein.
