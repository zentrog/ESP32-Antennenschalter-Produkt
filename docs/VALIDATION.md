# Prüfprotokoll – Produktversion 1.8.5

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

PlatformIO Core 6.1.19, Espressif32 6.12.0, Arduino-ESP32 2.0.17. Der lokale Build benötigt wegen eines schreibgeschützten IntelHex-Zusatzmoduls eine temporäre Python-Importanpassung unter `%TEMP%`; installierte PlatformIO-Dateien werden nicht verändert.

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

## Änderungen in 1.8.4

- Die Bedienansicht ordnet die Anlagenteile bei kürzeren Laptopfenstern und Telefonen automatisch neu an. Auf großen Ansichten bleibt das gespeicherte Raster erhalten.
- Die Bedienseite verwendet weiterhin die vorhandene Ein-Seiten-Ansicht ohne innere oder äußere Scrollbereiche. Schrift und Abstände passen sich an die verfügbare Fläche an; die Footerzeile bleibt sichtbar.
- Die Fußzeile zeigt wieder den vom Betreiber ausdrücklich freigegebenen Namen und die E-Mail-Adresse.
- Konfigurationsdaten, WLAN-Daten, Rufzeichen, Postleitzahlen, Geräte, Signalwege und GPIO-Belegungen wurden nicht verändert oder in den Release eingebettet.
- Cache-Kennungen der Weboberfläche auf v1.8.4 angehoben.
- Das OTA-Paket enthält Firmware, `index.html`, `app.js` und `responsive.css` in einer einzigen Datei. Die drei Webdateien liegen Brotli-komprimiert und CRC32-geprüft im freien Ende derselben Programmpartition. Die Firmware liefert diese Dateien nach einem gültigen Prüfsummenabgleich direkt aus dem App-Slot aus.
- Das bisherige Updateformular bleibt kompatibel: Es schreibt die eine, auf die volle OTA-Slotgröße aufgefüllte Datei in die inaktive App-Partition. Der Build bricht ab, wenn Firmware, Oberflächenpaket und Endmarkierung nicht vollständig hineinpassen. LittleFS wird nicht aktualisiert und Konfigurationsdateien bleiben unangetastet.

## Buildstatus 1.8.4

| Prüfung | Ergebnis |
|---|---|
| PlatformIO `esp32dev` Firmware | PASS; 1.446.809 / 1.507.328 Byte Flash, 52.960 / 327.680 Byte RAM |
| PlatformIO `esp32dev-jungfrau` Firmware | PASS; 1.445.233 / 1.507.328 Byte Flash, 52.960 / 327.680 Byte RAM |
| LittleFS-Erstabild `esp32dev` und `esp32dev-jungfrau` | PASS; enthält nun getrennte Basis- und Responsive-CSS-Dateien |
| Vollständiges OTA-Paket `esp32dev` | PASS; genau 1.507.328 Byte; Roh-Firmware 1.453.392 Byte; Webdateien 48.268 Byte; 5.632 Byte Reserve vor Endmarkierung |
| Vollständiges OTA-Paket `esp32dev-jungfrau` | PASS; genau 1.507.328 Byte; Roh-Firmware 1.451.808 Byte; Webdateien 48.268 Byte; 7.216 Byte Reserve vor Endmarkierung |
| SHA-256 `firmware-esp32dev.bin` | `5da76afe3f3a55d38cdcb9a2bcca5ce451015a6efe2d19281f46fe4454432d23` |
| SHA-256 `firmware-esp32dev-jungfrau.bin` | `5210e0dc02d61465bf77440ab249e1b87965e93a1a318c4bff8565a2b0147343` |
| Brotli-Entpacken, Inhalt und CRC32 der Paketdateien | PASS; der Paketbau prüft die drei extrahierten Dateien gegen die Quellvorlagen |
| ESP32-Imageprüfung des vollständigen Pakets | PASS; beide Images besitzen gültigen ESP32-Checksum- und SHA-256-Wert, esptool 4.9.0 |
| Öffentliche Quellenprüfung | PASS; im vollständigen erreichbaren Git-Verlauf keine Treffer für bekannte WLAN-Namen, Betreiber-Rufzeichen, PLZ oder private Geräte-IP; Konfigurationsdateien sind nicht Teil des Pakets |
| Manuelles OTA von GitHub auf echtem ESP | OFFEN; Veröffentlichung und Nutzerlauf stehen noch aus |
| Smartphoneansicht am echten Gerät | OFFEN; Desktop-Testansichten sind kein Ersatz für den Handytest |

## Änderungen in 1.8.5 (Arbeitsstand)

- Sicherungsdateien erhalten einen Dateinamen mit Controller-ID und aktueller IP-Adresse. Dieselbe Zuordnung wird im JSON-Inhalt mitgeführt.
- Sicherungsdialoge erklären direkt am Download: Die Sicherung umfasst den ESP, dessen eigene Oberfläche geöffnet wurde, plus gemeinsame Anlagenkonfiguration und dessen WLANs. Für eine vollständige Verbundsicherung wird jeder ESP einzeln auf seiner eigenen Adresse gesichert.
- Die Bedienoberfläche behält die gespeicherten 12×6-Rasterpositionen über Bildschirmgrößen hinweg bei. Responsive Regeln skalieren die Rasterfläche und Beschriftungen, ohne Karten automatisch neu anzuordnen.
- Vier sichtbar angelegte Reihen wie in der gespeicherten Benutzeranordnung bleiben erhalten; vier Reihen werden nicht global erzwungen.
- Vollständiges OTA-Paket enthält Firmware und alle geänderten Webdateien in einer Datei. LittleFS mit den vorhandenen Einstellungen wird beim OTA nicht geschrieben.
- Es gibt nur noch den Build `esp32dev` und eine OTA-Datei. Die frühere Jungfrau-Variante wurde aus dem aktiven Build und den aktuellen Download-Anweisungen entfernt.
- Die Schaltfläche „Neueste Firmware direkt herunterladen“ verwendet GitHub Releases/latest/download mit dem festen einzigen Assetnamen. Sie öffnet keine Release-Auswahlseite.

## Buildstatus 1.8.5 (Arbeitsstand)

| Prüfung | Ergebnis |
|---|---|
| PlatformIO `esp32dev` plus vollständiges OTA-Paket | PASS; Firmware 1.449.237 / 1.507.328 B, 52.960 / 327.680 B RAM; Paket 1.507.328 B mit Firmware 1.455.808 B, Webdateien 48.537 B und 2.947 B Reserve |
| SHA-256 `firmware-esp32dev.bin` | `f789f44ef19942126bb72bf9252c8443f3f72f686e79e4a3505be408a36a8af5` |
| ESP32-Imageprüfung | PASS; `esptool` 4.9.0 meldet gültige Image-Prüfsumme und SHA-256 für die vollständige 1.507.328-Byte-Datei |
| GitHub-Veröffentlichung und OTA auf echtem ESP | OFFEN |
| Positionsgleichheit und Scrollfreiheit auf echten Handy-, Laptop- und 4K-Ansichten | OFFEN; noch keine Sichtprüfung auf diesen Geräten |

| Prüfung | Ergebnis |
|---|---|
| GitHub-Hauptzweig enthält v1.8.4 | PASS; Release-Tag zeigt auf den veröffentlichten Stand |
| GitHub-Release v1.8.4 mit beiden damaligen OTA-Dateien | PASS; historische Release-Fassung |

## Schutzregeln für den manuellen Updateversuch

- Vor dem OTA die Sicherungsdatei auf den Computer herunterladen und speichern. Sie enthält WLAN-Kennwörter und muss privat bleiben.
- Nur die passende vollständige OTA-Datei hochladen. Die Datei enthält Firmware und Oberfläche; sie aktualisiert nicht LittleFS.
- Nach dem Neustart Version, Master-/Follower-Rolle, Rufzeichen, PLZ, WLAN, Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl und Peer-Status kontrollieren.
- Die Sicherungsdatei aufbewahren, bis alle Einstellungen nach dem Update geprüft sind.

## Datenschutz vor Veröffentlichung

Prüfe vor dem Upload Quelltext, Dokumente, Binärdateien, Anhänge und erreichbare Git-Historie auf Zugangsdaten, personenbezogene Angaben, KI-Dienste und Geräte-Konfigurationen. Veröffentlichte Prüfsummen müssen exakt zu den angehängten Release-Dateien gehören.

