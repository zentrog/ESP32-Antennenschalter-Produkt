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
5. Kennzeichne nicht ausgeführte Prüfungen als offen. Ein Build beweist keine Funktion am Gerät.
6. Vor einem öffentlichen Upload alle freizugebenden Dateien und Release-Dateien prüfen. Keine private Alt-Historie oder Sicherungsverzeichnisse übernehmen.

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
