# ESP32-Antennensteuerung

Eine browserbasierte Steuerung für ESP32, Relais, Funkgeräte und Antennen. Ein einzelner ESP kann allein arbeiten. Mehrere ESPs können optional als Master und Slaves zusammenarbeiten.

Aktuelle stabile Firmware: **1.8.22**. Das Release enthält genau eine vollständige OTA-Datei: `firmware-esp32dev.bin`. [Release v1.8.22 herunterladen](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/tag/v1.8.22) · [Firmware direkt herunterladen](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/latest/download/firmware-esp32dev.bin).

Unter **Diagnose** gibt es ein freiwilliges Fehler-/Wunschformular mit sicherer Vorschau und lokalem Download-Fallback. Der komplette Versandweg vom ESP-Formular bis zum E-Mail-Empfang wurde am 10.10.2026 mit Firmware 1.8.10 bestätigt. Der Bericht wird erst nach ausdrücklicher Bestätigung versendet und enthält keine Gerätekonfiguration. Fehlerfälle und der lokale Download-Fallback bleiben gesondert zu prüfen; Details stehen unter [Melde-Endpunkt und Freigabestatus](docs/REPORT-ENDPOINT.md).

Die gespeicherte Anordnung der Anlagenteile bleibt auf Handy, Laptop und großem Monitor an denselben Rasterpositionen. Die vollständige Bedienseite samt Schrift, Gerätekarten, Wetter, Newsticker und Footer wird gleichmäßig an den verfügbaren Browserbereich angepasst; Anlagenteile werden dabei nicht automatisch umgeordnet.

Eine Neuinstallation enthält sieben öffentliche NewsTicker-Quellen. Standardmäßig ist nur **n-tv Topmeldungen** eingeschaltet; die anderen Quellen können in **Konfigurieren → NewsTicker** einzeln aktiviert werden.

Stromtaster zeigen ihren bestätigten Zustand über die Farbe der Statusfläche. Die Farben für aktive und inaktive Stromtaster lassen sich getrennt unter **Konfigurieren → Oberfläche** einstellen.

Die Release-Datei `firmware-esp32dev.bin` enthält Firmware und die vier Hauptdateien der Weboberfläche in einem CRC32-geprüften Paket. Nach erfolgreicher Paketprüfung entfernt der ESP deren veraltete Kopien aus LittleFS und liefert die Dateien nur noch aus dem Firmwarepaket aus. Einzeln hochgeladene UI-Dateien werden dann abgewiesen. WLAN, Gerätekonfiguration, Sicherungen, Einrichtungsseite, Logo und Favicon bleiben erhalten.

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

Die Weboberfläche prüft GitHub Releases auf die neueste stabile Version. Wird eine neuere Version gefunden, startet **Jetzt aktualisieren** mit einem Klick zuerst den Download einer geprüften Sicherung dieses ESPs und danach die Firmwareinstallation direkt aus GitHub. Es muss keine Firmwaredatei mehr gesucht oder ausgewählt werden. Die Sicherung wird im Browser heruntergeladen; zusätzlich speichert der ESP vor dem Flashen eine interne Sicherung. Die LittleFS-Datenpartition mit der Konfiguration wird nicht formatiert.

Eine Sicherungsdatei enthält die lokalen Einstellungen des geöffneten ESP, die gemeinsame Anlagenkonfiguration und dessen gespeicherte WLANs. Im Verbund sammelt der Master die lokalen Einstellungen der Slaves nicht ein: Öffne deshalb jeden ESP über seine eigene Adresse und sichere ihn separat. Die automatische Ein-Klick-Aktualisierung aktualisiert nur den geöffneten ESP. Controller-ID und IP stehen im Sicherungsdateinamen und in der Datei.

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
