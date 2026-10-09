# Prüfprotokoll – Produktversion 1.8.0

Alte Ergebnisse anderer Versionen gelten nicht als Nachweis für diese Produktversion.

## Änderungen an der Produktversion

- Neue Geräte und Factory-Resets starten laut Quelltext ohne private Identität, WLAN-Zugangsdaten, Relais, Funktionen oder Geräte.
- Entwicklungs-WLANs und Standard-Beispielgeräte wurden aus dem aktuellen Produktpfad entfernt.
- Die Onlineprüfung ist auf das neueste stabile Release des Produkt-Repositories eingestellt und installiert nichts automatisch. Das Ziel-Repository existiert noch nicht; der Live-Aufruf ist daher OFFEN.
- Normale Firmware-Updates müssen bestehende Gerätekonfigurationen erhalten.
- Ein fehlgeschlagener LittleFS-Mount darf nicht automatisch formatieren; neue Geräte erhalten das Dateisystem ausdrücklich beim Erstflash.

## Aktueller Prüfstand

| Prüfung | Ergebnis |
|---|---|
| Datenschutzscan der vorgesehenen Quell-, Dokument- und Release-Dateien | PASS; alte Sicherungsordner ausgeschlossen |
| Bestehendes GitHub-Repository für öffentliche Nutzung | NICHT GEEIGNET; getrennte Historie nötig |
| esp32dev-Build | PASS |
| esp32dev-jungfrau-Build | PASS |
| LittleFS-Abbild esp32dev-jungfrau | PASS |
| Statische Prüfung der leeren Startwerte | PASS |
| Erststart mit leerem Testgerät | OFFEN |
| Updateprüfung gegen öffentliches GitHub-Release | OFFEN; das Ziel-Repository ist noch nicht angelegt |
| OTA mit Vorher-/Nachher-Konfigurationsvergleich | OFFEN |
| Öffentliche Repository-Sichtbarkeit | OFFEN |

## Buildnachweis

- Datum: 09.10.2026
- PlatformIO Core 6.1.19; Espressif32 6.12.0; Arduino-ESP32 2.0.17.
- Beide Umgebungen wurden mit PlatformIO Core gebaut. RAM jeweils 52.912 / 327.680 Byte (16,1 %).
- Flash esp32dev: 1.439.009 / 1.507.328 Byte (95,5 %).
- Flash esp32dev-jungfrau: 1.437.417 / 1.507.328 Byte (95,4 %).
- LittleFS-Abbild wurde für esp32dev-jungfrau erfolgreich erzeugt.
- Für esptool wurde eine temporäre, projektlokale Python-Startanpassung verwendet, die die installierte IntelHex-Kopie vor einer schreibgeschützten Vendor-Kopie lädt. Keine installierten Berechtigungen wurden geändert.
- Der Firmware-Binärscan fand vier eindeutige Treffer eines allgemeinen `sk-`-Musters. Jeder Treffer wurde in der unveränderten Espressif-MbedTLS-Archivdatei `libmbedtls_2.a` wiedergefunden; die Muster stehen nicht im Projektquelltext. Der gezielte Scan fand keine bekannten API-Schlüssel, E-Mail-Adressen, Rufzeichen, WLAN-Namen oder privaten Geräte-IP-Adressen.

## Schutzregeln für bestehende Geräte

- Vorher und nachher vergleichen: Firmwareversion, Master-/Slave-Rolle, Rufzeichen, Postleitzahl, WLAN-Namen (niemals Kennwörter), Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl, Motor, TX und Peer-Status.
- Keine Konfigurations-API, keinen Factory-Reset und keinen LittleFS-Upload verwenden.
- Bei einer Abweichung sofort stoppen. Keine automatische Wiederherstellung oder Änderung an der Konfiguration versuchen.

Nach einem erfolgreichen Lauf werden Datum, Commit, Board, PlatformIO-/Frameworkversion, Werkzeugausgabe und bereinigte Vorher-/Nachher-Zahlen ergänzt. Kennwörter und andere private Konfigurationswerte gehören nie in dieses Protokoll.
