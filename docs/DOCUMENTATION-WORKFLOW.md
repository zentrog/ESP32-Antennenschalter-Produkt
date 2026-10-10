# Entwicklungs-, Dokumentations- und Veröffentlichungsablauf

## Maßgebliche Bestandteile

- Der Quellcode, der tatsächlich gebaut und geprüft wurde, ist die Referenz für Firmwareänderungen.
- GitHub ist der versionierte und später veröffentlichte Quellstand.
- Hardwareergebnisse gelten nur für die dabei gebaute Version. Versionsnummer und Commit müssen zum Prüfprotokoll passen.

## Bei jeder Änderung

1. Schreibe auf, was geändert wurde und welche Gerätefunktionen betroffen sind.
2. Halte README, Anwenderanleitung, Projektstatus und passende technische Dokumente auf derselben Versionsnummer.
3. Bei Änderungen an Firmware, Oberfläche, Build-Einstellungen oder Partitionen Manifest und Prüfprotokoll aktualisieren.
4. Baue die betroffenen PlatformIO-Umgebungen und notiere tatsächliches Ergebnis und Speicherverbrauch.
5. Für die einzige ESP32-DevKit-Veröffentlichung baue `node tools/build-ota-package.mjs esp32dev`. Die erzeugte OTA-Datei muss Firmware und alle geänderten Webdateien enthalten; eine separate UI-Datei genügt nicht. Der Paketbauer verweigert den Release, wenn sich `style.css` ändert, ohne dass das OTA-Paketformat dafür erweitert wurde.
6. Kennzeichne nicht ausgeführte Prüfungen als offen. Ein Build beweist keine Funktion am Gerät.
7. Vor einem öffentlichen Upload alle freizugebenden Dateien und Release-Dateien prüfen. Keine private Alt-Historie oder Sicherungsverzeichnisse übernehmen.
8. Für einen Produktrelease genau ein stabiles GitHub-Release aus dem geprüften `main`-Stand erstellen und ausschließlich `release-assets/firmware-esp32dev.bin` anhängen. Die Datei muss Firmware und alle geänderten Webdateien enthalten.
9. Nach der Veröffentlichung über die öffentliche Release-API prüfen: neuester Tag, stabiler Status, genau ein Asset, erwartete Dateigröße. Danach `/releases/latest/download/firmware-esp32dev.bin` anonym abrufen und SHA-256 mit dem lokalen Paket vergleichen.
10. Release- und Downloadprüfung mit Datum und Firmware-Quellcommit in `docs/VALIDATION.md` eintragen. OTA am echten Gerät und Ansichten auf echten Bildschirmgrößen bleiben eigene offene Prüfungen, bis sie ausgeführt wurden.
11. Wenn der Release eine externe E-Mail-Funktion enthält, den konkreten HTTPS-Endpunkt und Mailversand vor der Aktivierung Ende zu Ende prüfen. Nicht erreichbare externe Dienste müssen von der Oberfläche erkannt werden; ein funktionierender lokaler Download-Fallback bleibt erforderlich.

## Mindestangaben im Prüfprotokoll

- Produktversion und Commit SHA
- ESP32-Platine und PlatformIO-Umgebung
- PlatformIO-, Plattform- und Frameworkversion
- ausgeführte Aktion mit PASS, FAIL oder OFFEN
- RAM- und Flashverbrauch des Builds
- bei Hardwareprüfungen: Gerätezahl, Rollen, Vorher-/Nachhervergleich und bekannte Grenzen
- keine Kennwörter, persönlichen Rufzeichen, Adressen, privaten IPs oder Konfigurationsdateien

## Begriffe

- **STATIC CHECK:** Der untersuchte Quelltext oder das Manifest wurde geprüft.
- **BUILD PASS:** Die angegebene PlatformIO-Umgebung wurde ohne Fehler gebaut.
- **SIMULATION PASS:** Das dokumentierte Szenario lief in einer Simulation erfolgreich.
- **REAL-HARDWARE PASS:** Die dokumentierte Prüfung wurde am ESP ausgeführt und per Readback bestätigt.

## Leere Produktinstallation

Eine neue Installation und ein Factory-Reset dürfen keine Testzugänge, persönlichen Identitäten oder Beispielgeräte erhalten. WLAN-Zugangsdaten gibt der Anwender selbst am Gerät ein. Ein normales Firmware-OTA darf vorhandene Geräteeinstellungen nicht zurücksetzen.

## GitHub-Release und Updateprüfung

Die Weboberfläche liest die neueste stabile Veröffentlichung aus der GitHub-Release-Schnittstelle. Sie meldet nur eine verfügbare Version; die Installation bleibt eine bewusste Benutzeraktion. Vor einem Release Firmwaredatei und Versionskennung gemeinsam prüfen. Private oder unbestätigte Dateien nie als Release hochladen.

## Öffentliche Veröffentlichung

Ein privates Repository mit personenbezogenen Daten in alten Commits kann nicht dadurch bereinigt werden, dass nur der neueste Dateistand geändert wird. Falls frühere Daten in erreichbaren Branches oder Tags liegen, einen neuen sauberen Veröffentlichungsstand ohne diese Historie verwenden und die alte Ablage privat lassen. Die Sichtbarkeit erst nach erfolgreicher Prüfung ändern.
