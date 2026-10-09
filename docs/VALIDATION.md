# Prüfprotokoll – Produktversion 1.8.3

Dieses Protokoll unterscheidet Quelltext- und Buildprüfungen von noch ausstehenden Prüfungen an echten Geräten. Ein erfolgreicher Build beweist keine OTA-Funktion.

## Änderungen in 1.8.1

- Unvollständige Konfigurationsanfragen werden abgewiesen; vorhandene Relais, Funktionen, Geräte, Anordnung und Signalwege sind gegen versehentlich leere Listen geschützt.
- Gespeicherte Signalwege werden zusätzlich als vorherige Generation auf dem ESP vorgehalten.
- Vor manuellen Firmware- und Rückschaltvorgängen muss eine geprüfte Sicherungsdatei heruntergeladen und bestätigt werden. Sie enthält lokale und gemeinsame Konfiguration sowie WLAN-Zugangsdaten.
- Updateprüfung und Downloadseite verwenden GitHub. Die Prüfung installiert keine Firmware automatisch.
- Nach einem erfolgreichen Wetterabruf beträgt das Abrufintervall 30 Minuten. Nach einem fehlgeschlagenen Abruf wird nach 5 Minuten erneut versucht; bis dahin bleibt ein vorhandener gültiger Stand sichtbar.
- Die Bedienoberfläche zeigt eine einheitliche Schriftgröße, eine größere YAGI-Fläche für H/V, die Firmwareversion im Kopfbereich und besser lesbare Fußzeilenangaben.

## Änderungen in 1.8.2

- Die GitHub-Updatehinweise erscheinen als kurze Zusammenfassung der ersten beiden Aufzählungspunkte statt als kompletter Markdown-Block.
- Versionshinweise werden sicher als Text eingesetzt und nicht als HTML interpretiert.
- Der Cache-Schlüssel der Weboberfläche wurde auf 1.8.2 angehoben.

## Änderungen in 1.8.3

- Die Laufzeitbegrenzung zeitgesteuerter Relaisfunktionen wurde von 30 auf 100 Sekunden erhöht. Die Oberfläche, Konfigurationsprüfung und Motorsteuerung verwenden dieselbe Obergrenze.
- Die bestehenden gespeicherten Laufzeiten werden nicht automatisch verändert.

## Buildstatus 1.8.3

- C++-Kompilierung und Linken von `esp32dev` und `esp32dev-jungfrau`: PASS.
- Erzeugung beider Firmwaredateien und des LittleFS-Abbilds: PASS.
- Eine temporäre `sitecustomize.py` im temporären Verzeichnis lädt IntelHex aus der PlatformIO-Python-Umgebung vor dem gesperrten Zusatzpfad. Installierte PlatformIO-Dateien wurden nicht verändert.

## Quellrevision der Release-Dateien

Firmware und LittleFS für 1.8.3 wurden aus Firmware-Quellcommit `015fb39ab048cf37b5b19a7b2d2d0f47d37589c7` gebaut. Der folgende Dokumentations- und Manifest-Commit ändert keine Firmwarequellen. Die SHA-256-Prüfsummen der drei Release-Dateien stehen im Manifest; die GitHub-Release-Dateien wurden nach dem Upload gegen Größe und SHA-256 geprüft.

## Buildstand

| Prüfung | Ergebnis |
|---|---|
| PlatformIO `esp32dev` | PASS; 1.444.821 / 1.507.328 Byte Flash (95,9 %), 52.920 Byte RAM (16,1 %) |
| PlatformIO `esp32dev-jungfrau` | PASS; 1.443.225 / 1.507.328 Byte Flash (95,7 %), 52.920 Byte RAM (16,1 %) |
| LittleFS-Abbild `esp32dev` für Erstinstallation | PASS |
| Flashverbrauch `esp32dev` | 1.444.821 / 1.507.328 Byte (95,9 %) |
| Flashverbrauch `esp32dev-jungfrau` | 1.443.225 / 1.507.328 Byte (95,7 %) |
| RAMverbrauch beider Umgebungen | 52.920 / 327.680 Byte (16,1 %) |

PlatformIO Core 6.1.19, Espressif32 6.12.0, Arduino-ESP32 2.0.17. Für den Build auf dieser Windows-Installation wurde eine temporäre lokale Python-Importanpassung verwendet, weil ein mitgeliefertes, schreibgeschütztes IntelHex-Modul sonst den Bootloader-Build verhindert. Es wurden keine installierten PlatformIO-Dateien geändert.

## Live-Stand und offene Prüfungen

| Prüfung | Ergebnis |
|---|---|
| Öffentliche Sichtbarkeit des Produkt-Repositories | PASS; GitHub-API meldet öffentlich |
| GitHub-Updateprüfung auf beiden ESPs | PASS; installiert `1.8.3`, GitHub meldet `v1.8.3`, kein neueres Update verfügbar |
| OTA 1.8.2 → 1.8.3 auf Master und Follower | PASS; beide Geräte starteten in ihrer bisherigen Rolle wieder |
| Konfiguration nach OTA | PASS; Relais, Funktionen, Geräte, Signalwege, Layout und WLAN-Einträge stimmen mit den vor dem OTA lokal gespeicherten Sicherungen überein |
| Weboberfläche nach OTA | PASS; beide Geräte liefern Cachekennung `1.8.3-motor-100s` und die 100-Sekunden-Eingabegrenze |
| Gerätespeicherungen vor OTA | PASS; zwei geprüfte Sicherungsdateien liegen lokal außerhalb des Repositorys |
| Physischer Motorlauf mit 50 Sekunden | OFFEN; auf den ESPs wurde keine Motorfahrt ausgelöst |

Gerätenamen, Rufzeichen, Postleitzahlen, WLAN-Namen und Kennwörter sowie lokale IP-Adressen werden hier nicht dokumentiert. Die Sicherungsdateien bleiben ausschließlich lokal.

## Schutzregeln für den manuellen Updateversuch

- Vor dem OTA die Sicherungsdatei auf den Computer herunterladen und speichern. Sie enthält WLAN-Kennwörter und muss privat bleiben.
- Nur die passende Firmwaredatei hochladen. Das LittleFS-Abbild überschreibt den Dateispeicher und ist ausschließlich für die Erstinstallation vorgesehen.
- Nach dem Neustart Version, Master-/Follower-Rolle, Rufzeichen, PLZ, WLAN, Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl und Peer-Status kontrollieren.
- Bei einer Abweichung anhalten. Keine Konfigurations-API, keinen Factory-Reset und keinen LittleFS-Upload verwenden.

## Datenschutz vor Veröffentlichung

Prüfe vor dem Upload Quelltext, Dokumente, Binärdateien, Anhänge und erreichbare Git-Historie auf Zugangsdaten, personenbezogene Angaben, KI-Dienste und Geräte-Konfigurationen. Veröffentlichte Prüfsummen müssen exakt zu den angehängten Release-Dateien gehören.

