# Prüfprotokoll – Firmware-Releases

## Firmware 1.8.16 · Stromkartenfarben

- Ziel: Stromkarten zeigen keinen sichtbaren EIN/AUS-Text. Der bestätigte Status wird allein durch die Farbe der Statusfläche vermittelt; die gesamte Karte bleibt anklickbar. In **Konfigurieren → Oberfläche** sind nun eigene Farben für inaktive und aktive Stromtaster verfügbar. Gesperrte Taster verwenden weiterhin die Farbe „Gesperrt“.
- Geänderte Bereiche: `include/Model.h`, `src/Storage.cpp`, `src/WebUi.cpp` speichern/laden die beiden zusätzlichen gemeinsamen Oberflächenfarben; `data/app.js` zeigt den farbigen Status ohne EIN/AUS-Schrift, ergänzt die barrierefreie Statusbeschreibung und die zwei Farbwähler; `data/index.html` erhält neue Cachemarker; `include/Version.h` setzt 1.8.16.
- Keine Änderungen an WLAN, Geräten, Relaiszuordnungen, Signalwegen, Layout, Newsquellen, GPIO-Schaltlogik oder OTA-Sicherungsverhalten.
- Quellcommit: `ccecd4edc52ac965a40a57e2beec42aec9a9fbc5` (`Release firmware 1.8.16`). Der dokumentierende Commit `deeac89ab500eb655f384f052afeb4dbf218cc4e` trägt den Release- und Asset-Abgleich nach.
- Build: `PASS` mit Link-Time-Optimierung (`-flto`), PlatformIO Core 6.2.0 / Espressif32 6.12.0 / Arduino-ESP32 3.20017.241212+sha.dcc1105b / Xtensa-GCC 8.4.0+2021r2-patch5 / esptool 4.9.0. RAM 52.448 / 327.680 Byte; Firmwareabbild 1.367.392 Byte. Komplettes OTA-Paket: 1.507.328 Byte, UI Brotli 63.057 Byte + gzip 74.709 Byte; Reserve 2.094 Byte. SHA-256 `A68856BEEACD62D8C9608E7BE48E5A1EA5C467471706DAA4633C37FAC4E88624`.
- OTA-Test am ausdrücklich vorgesehenen Testgerät: `PASS`, Firmware 1.8.15 → 1.8.16, HTTP 200 und erfolgreicher Neustart. Der Browser zeigt danach v1.8.16. Die Bedienseite rendert vier Stromkarten ohne sichtbares EIN/AUS; der ganze Eintrag ist als Schaltfläche ausgezeichnet und sein barrierefreier Name nennt den bestätigten Zustand.
- Konfigurationserhalt: `PASS`. Vor und nach dem OTA erstellte vollständige Sicherungen wurden in `local`, `shared` und `wifiState` verglichen; alle drei Bereiche sind unverändert. Die neuen Standardfarben werden im gemeinsamen API-Readback als `#275c91` (inaktiv) und `#259b55` (aktiv) bereitgestellt. Vorher-/Nachher-Sicherungen bleiben nur lokal im gitignorierten `backups/config`-Ordner.
- Livebeobachtung nach dem Neustart: Wetter wurde angezeigt und n-tv lieferte eine Meldung. Das ist eine Momentaufnahme und kein Nachweis für dauerhafte Feedverfügbarkeit oder eine Fehlerursache.
- Der Betreiber hat bestätigt, dass die Farbauswahl funktioniert. Einen Neustarttest mit selbst gewählten Farben haben wir nicht separat ausgeführt.
- Die Partitionstabelle weist zwei gleich große OTA-Programmplätze von je `0x170000` (1.507.328 Byte) aus. OTA überschreibt stets den inaktiven Platz und behält den bisherigen als Rückfallversion. Die Plätze addieren sich nicht zur maximalen Imagegröße. LTO ist bereits aktiv; das aktuelle vollständige Image passt, die Reserve beträgt aber nur 2.094 Byte. Eine Vergrößerung der Plätze erfordert eine riskante Änderung der Partitionstabelle zulasten des 1-MiB-LittleFS-Bereichs und ist kein sicheres OTA-Upgrade.
- GitHub-Release: `PASS`, öffentlich und neueste stabile Version [v1.8.16](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/tag/v1.8.16). Annotierter Tag `v1.8.16` zeigt auf Release-Commit `deeac89ab500eb655f384f052afeb4dbf218cc4e`; Firmwarequellcommit `ccecd4edc52ac965a40a57e2beec42aec9a9fbc5`. Genau ein Asset `firmware-esp32dev.bin`, 1.507.328 Byte. Anonymer Direktdownload `/releases/latest/download/firmware-esp32dev.bin`: 1.507.328 Byte; SHA-256 `A68856BEEACD62D8C9608E7BE48E5A1EA5C467471706DAA4633C37FAC4E88624`, stimmt vollständig mit dem lokalen Paket überein. Prüfung am 10.10.2026.
- Anwenderhinweis: „Fehlerbehebungen und Stabilitätsverbesserungen. Stromtaster: Statusfarben einstellbar.“

## Firmware 1.8.15 · NewsTicker-Standardquellen und Stromkarten

- Ziel: eine jungfräuliche Produktinstallation soll die sieben öffentlichen NewsTicker-Quellen enthalten, ohne private Stationsdaten, WLAN-Daten oder Beispielgeräte. Nur n-tv ist ab Werk aktiviert; die übrigen sechs Quellen sind vorhanden und ausgeschaltet. Stromkarten sollen den Namen nicht ein zweites Mal im Ein/Aus-Element zeigen; die ganze Karte schaltet und der Status steht zentriert unten.
- Fehlerursache: die Erstinstallations-/Factory-Reset-Routine leerte `news.feeds`, schaltete den NewsTicker aus und markierte zugleich die einmaligen Datenmigrationen als erledigt. Die Migrationen konnten die Standardquellen danach nicht mehr ergänzen. Auf dem Live-Testgerät waren vor der Korrektur 0 Quellen gespeichert. Die vorliegenden alten Sicherungen hatten ebenfalls bereits 0 Quellen; sie enthielten keine ältere Feedliste zur Wiederherstellung.
- Live-Konfiguration repariert: vor Änderung vollständige ESP-Sicherung und lokale API-Kopie abgelegt (ignorierte Dateien unter `backups/config`, Controller-ID ESP32-28FDE2842178). Danach sieben Standardquellen eingetragen; Rücklesung zeigt alle sieben und ausschließlich n-tv aktiviert. Geänderte News-Einstellung wurde gespeichert, Revision 10. Abruf-Ergebnis der n-tv-Quelle: `OFFEN`, solange der ESP keinen erfolgreichen Quellenabruf meldet.
- Quelländerungen: `src/Storage.cpp` liefert die sieben öffentlichen Standardquellen in jungfräulichen und zurückgesetzten Konfigurationen; `data/app.js`/`data/style.css` vereinfachen und vergrößern die Bedienfläche der Stromkarten. Kein WLAN, Rufzeichen, PLZ, Relais, Gerät, Signalweg oder Anlagenlayout wird durch diese Änderungen vorbelegt.
- Anwenderhinweis: „Fehlerbehebungen und Stabilitätsverbesserungen. Standard-Newsquellen sind wieder vorhanden; nur n-tv ist eingeschaltet.“
- Build: `PASS`, PlatformIO Core 6.2.0 / Espressif32 6.12.0 / Arduino-ESP32 3.20017.241212+sha.dcc1105b / Xtensa-GCC 8.4.0+2021r2-patch5 / esptool 4.9.0. RAM 52.384 / 327.680 Byte; Programmabbild 1.367.696 Byte. OTA-Paket: 1.507.328 Byte, Brotli 62.870 Byte + gzip 74.539 Byte, Restreserve 2.147 Byte; SHA-256 `8E4DFEC0DBCA83C18C2174B664B1ED31771BADCA9333026DBC9C1F97707DF602`.
- Paketgröße ist knapp: Das vollständige Image passt, aber es bleiben nur 2.147 Byte Reserve im OTA-Slot. Vor einer künftigen Erweiterung ist weiterer Flash-Platz freizumachen oder der Paketumfang zu optimieren.
- Browserbedienung der Stromkarte, OTA auf ESP, GitHub-Veröffentlichung und anonymer Direktdownload: `OFFEN` bis die jeweiligen Schritte tatsächlich erfolgreich geprüft wurden.

## Firmware 1.8.14 · Stromtaster

- Funktion: Kategorie **Strom** für eigenständige Geräteschalter; pro Stromgerät kann ein eigener EIN/AUS-Taster angelegt, benannt und später einem Relais/GPIO zugeordnet werden. Der Taster ist unabhängig von Signalwegen. Ohne zugeordnetes Relais bleibt er sichtbar, aber gesperrt.
- Zustandslogik: Ein/Aus-Funktionen erhalten eine eigene persistente Gruppe und verwenden den bereits vorhandenen NVS-Speicher für aktive Auswahlen. Ein Neustart stellt einen gespeicherten EIN-Zustand erst dann wieder her, wenn die konfigurierte Relaiszuordnung verfügbar ist. Stromtaster dürfen keine Antennengruppe teilen.
- Build: PASS mit PlatformIO Core 6.2.0, Espressif32 6.12.0, Arduino-ESP32 3.20017.241212+sha.dcc1105b, Xtensa-GCC 8.4.0+2021r2-patch5 und esptool 4.9.0. RAM 52.384 / 327.680 Byte; Firmware 1.364.976 Byte. Vollständiges OTA-Paket `firmware-esp32dev.bin`: 1.507.328 Byte, UI Brotli 62.462 Byte + gzip 74.063 Byte, Restreserve 5.751 Byte. Paket-SHA-256 `515822A52344D4EF8D1F5DBE280CABC2E2EEF94B1A16DA03EAA5BD70A0EB03FA`.
- OTA: PASS, Testgerät 1.8.13 → 1.8.14 über die Weboberfläche. Sicherungsdownload vor dem Upload, OTA-Antwort HTTP 200. Danach gleiche Gerätekennung, IP und Master-Rolle; Firmware 1.8.14 / API 7 online.
- Konfigurationserhalt: Vorher-/Nachher-Sicherung verglichen. Unverändert blieben alle nicht angefassten lokalen Felder, gemeinsame Konfigurationsfelder und der vollständige WLAN-Zustand. Die 16 bestehenden Signalwege blieben erhalten. Die vorhandenen 8 Geräte wurden um 4 Stromgeräte ergänzt; die gemeinsame Anordnung wuchs von 8 auf 12 Rasterelemente.
- Gerätekonfiguration: Vier Geräte **Strom Funkgerät 1–4** und vier individuelle `toggle`-Funktionen angelegt. Ein frischer Sicherungsexport enthält 12 Geräte, 4 Stromtaster, 16 Signalwege und 12 Layoutelemente. Die Steuerseite bestätigt alle vier Taster als sichtbar und mit „GPIO fehlt“ gesperrt. Das Gerät besitzt derzeit keine Relais/GPIO-Zuordnungen; echte Relaisbetätigung und Stromwiederherstellung mit angeschlossener Hardware sind daher noch nicht geprüft.
- Sicherungsschutz-Prüfung: Der erste Readback-Export enthielt die neue Anordnung noch nicht. Die vollständige gemeinsame Konfiguration wurde erneut gespeichert; danach stimmten Live-API und ein frisch erstellter Sicherungsexport mit 12 Layout-Elementen und allen 16 Signalwegen überein. Der finale Export wurde außerhalb des Repositories auf dem Desktop abgelegt.
- Release-Hinweis für Anwender: „Funktion: Eigenständige Stromtaster für Geräte.“
- GitHub-Release v1.8.14 und öffentlicher Direktdownload: `PASS`, siehe späteren vollständigen Release-Nachweis in diesem Protokoll.

## Frühere Release-Prüfprotokolle

### Firmware 1.8.13

- Ursache der unvollständigen Sicherungswiederherstellung: Der UI-Import sendete die gespeicherten lokalen und gemeinsamen JSON-Objekte, aber `apiConfigPut()` übernahm aus dem lokalen Federation-Objekt nur `enabled`. System-ID, Systemname, UDP-Port, Rolle, permanenter Master, Admission-Modus und Priorität blieben vom frisch gestarteten Zustand erhalten.
- Korrektur: Der vollständige Federation-Block wird jetzt eingelesen und gespeichert; Änderungen an beliebigem Federation-Feld lösen `FederationService::configChanged()` aus. Feldwerte werden nicht öffentlich protokolliert. Die Sicherungsdatei wird nicht automatisch importiert.
- Restore-Test am 10.10.2026: Betreiber importierte die ursprüngliche Sicherung nach Installation von 1.8.13 manuell über die Weboberfläche. Live-Readback der neu erzeugten vollständigen Sicherung stimmt in `local`, `shared` und `wifiState` mit der Originalsicherung überein; lokale Revisionsnummer (4→5) und gemeinsame Revisionsnummer (4→13) wurden erwartungsgemäß fortgeschrieben. Federation-System-ID, Anlagenname, Master-Rolle, permanenter Master und UDP-Port stimmen mit dem Original überein. Alle 8 Geräte, 16 Signalwege und 1 WLAN-Eintrag sind vorhanden. Restore-Test: PASS.
- Build: PASS mit PlatformIO Core 6.2.0, Espressif32 6.12.0, Arduino-ESP32 3.20017.241212+sha.dcc1105b und Xtensa-GCC 8.4.0+2021r2-patch5. RAM 52.384 / 327.680 Byte; Firmware 1.363.776 Byte. Vollständiges OTA-Paket: 1.507.328 Byte, UI Brotli 61.758 Byte + gzip 73.163 Byte, Reserve 8.555 Byte. Paket-SHA-256 `CE6E86C8451E481E8698BFB4D9AE5D6B0113C7C9414DB023BC71CC9755B940B9`. Release: PASS, Quellcommit `1995162ad423f5523c0ce266cc89f832680d1930`, Tag `v1.8.13`, öffentliches neuestes GitHub-Release mit genau einem Asset `firmware-esp32dev.bin`; anonymer Direktdownload in Größe und SHA-256 geprüft. OTA auf Testgerät COM13, 1.8.12→1.8.13: PASS; Controller-ID, IP, Master-Rolle und lokale/gemeinsame/WLAN-Konfiguration blieben während des Firmware-OTA unverändert. Wiederherstellung der Originalsicherung über die Weboberfläche und anschließender Live-Sicherungsvergleich: PASS (siehe Restore-Test oben). Releasehinweis: „Fehlerbehebung: Wiederherstellung übernimmt jetzt auch die Verbundzuordnung.“
# Prüfprotokoll – Firmware 1.8.12

- Änderungen vor dem Eingriff: Arbeitsbaum sauber auf Branch `v1.8.4-ota-bundle`; Release-Quelle war 1.8.11. Betroffen: `src/WebUi.cpp`, Versions-/Cachemarker, `README.md`, `ANLEITUNG.md`, `docs/ARCHITECTURE.md` und dieses Protokoll. WLAN, Gerätekonfiguration, Relais, Signalwege und LittleFS-Migration bleiben unangetastet.
- Ursache: Das inline JavaScript der manuellen Update-Seite war syntaktisch ungültig, weil der übersetzte Hinweistext mit einfachen Anführungszeichen in das Script eingesetzt wurde. Dadurch wurde der Klick-Handler nicht ausgeführt und das deaktivierte Sicherungshäkchen blieb gesperrt.
- Korrektur: Das Häkchen ist direkt bedienbar und steuert nur die Freigabe der Uploadfelder. Die ESP-seitige Sperre `updateBackupDownloaded` bleibt unverändert und lehnt Upload/Rollback weiterhin ab, bis `/api/update-backup` erfolgreich abgerufen wurde.
- Build: PASS; PlatformIO Core 6.2.0, Espressif32 6.12.0, Arduino-ESP32 3.20017.241212+sha.dcc1105b, Xtensa-GCC 8.4.0+2021r2-patch5, esptool 4.9.0. RAM 52.384 / 327.680 Byte; Firmware 1.362.640 Byte. Paket: PASS; 1.507.328 Byte, UI Brotli 61.758 Byte, gzip 73.162 Byte, Reserve 9.692 Byte. Paket-SHA-256 `6DF4144A7DC636721D72CED19DF9A000B2CC2BD0D134C77A5B22329E4F53A372`. Browser-JavaScript-Syntaxprüfung am Live-ESP für den Fehlerfall: FAIL bestätigt; korrigierter Quellhandler ist syntaktisch einfach und benötigt keine Textinterpolation. Quellcommit `40426e6f17f583a3a5495554e3200884918403b7`, annotierter Tag `v1.8.12` zeigt auf diesen Commit. GitHub-Release: PASS, öffentlich und neueste stabile Version, genau ein Asset `firmware-esp32dev.bin`; Größe und SHA-256 des anonymen Direktdownloads stimmen mit dem lokalen Paket überein. OTA auf Testgerät COM13 / Controller-ID-Suffix `842178`: PASS, 1.8.11 → 1.8.12; Readback gleiche ID, `192.168.0.154`, Master. Vorher-/Nachher-Sicherungen: lokale Konfiguration, gemeinsame Konfiguration und WLAN-Zustand jeweils byte-normalisiert identisch. Produktionsgeräte nicht angefasst. Live-Update-Seite 1.8.12: Häkchen ohne `disabled`, JavaScript `node --check` PASS, Datei-Upload anfangs weiterhin gesperrt. Der frühere Live-Seitenfehler ist damit technisch reproduziert und im Quellcode behoben; Bediener bestätigte am 10.10.2026: Sicherung herunterladen, Häkchen setzen, Firmwaredatei laden und einspielen funktionieren. Eine zweite Update-Seitenöffnung ließ das Häkchen ohne erneuten Klick auf den Sicherungslink setzen; das ist durch den jetzt aktivierten Checkbox-Startzustand erklärbar und nicht automatisch ein Browsercache-Fehler. Die Checkbox schaltet nur Browserfelder frei; der ESP weist einen Upload ohne Sicherungsabruf in der aktuellen Update-Sitzung serverseitig zurück. Der Bediener meldete den Ablauf zunächst als erfolgreich. Der Live-Vergleich danach zeigte jedoch, dass die interne `federation.systemId` nicht wiederhergestellt wurde; die Wiederherstellung unter 1.8.12 war daher nur teilweise erfolgreich. Der gemeinsame Revisionszähler wird bei jedem Import erwartungsgemäß erhöht. Anwenderhinweis: „Fehlerbehebung: Sicherungshäkchen ließ sich nach Download nicht aktivieren.“
- Bediennachweis: `PASS` laut Betreiber; Sicherungsdownload, Häkchen, Firmwaredownload und Installation erfolgreich. Die vorherige Firmware wurde dabei nicht gestartet.
# Prüfprotokoll – Firmware 1.8.11

- Neueste stabile Firmware: **1.8.11**, GitHub-Release [v1.8.11](https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/tag/v1.8.11). API 6; genau ein Produkt-Asset `firmware-esp32dev.bin`.
- Firmware-Quellcommit und Release-Tag: `3d111b4d4701cccbedfbf3dafca8e8c8ec556ac5` / `v1.8.11`. Die nachträgliche Dokumentationsaktualisierung auf `main` verändert das Release-Tag nicht.
- Anwenderhinweis: „Fehlerbehebung: Sicherungsdatei ließ sich auf der manuellen Update-Seite teils nicht herunterladen.“
- Build: PASS; PlatformIO Core 6.2.0, Espressif32 6.12.0, Arduino-ESP32 3.20017.241212+sha.dcc1105b, Xtensa-GCC 8.4.0+2021r2-patch5, esptool 4.9.0. Node.js 24.19.0, npm 11.17.0; minifiziertes JavaScript-Syntaxchecking PASS.
- Speicher: RAM 52.384 / 327.680 Byte; Firmwareabbild 1.363.040 Byte.
- Vollständiges OTA-Paket: PASS; 1.507.328 Byte im festen Slot 1.507.328 Byte. Enthält Firmware sowie Brotli 61.757 Byte und gzip 73.162 Byte für die vier Hauptoberflächendateien; verbleibende Reserve 9.293 Byte. Paketprüfer: Footer, CRC32 und komprimierte Inhalte PASS.
- OTA-Datei SHA-256: `0A898194B8614BB7A1F1C25B4EED92C3A5192F87736DCDAA619CEB6C7307411F`.
- Änderung: Der Button auf der manuellen Update-Seite ist jetzt ein normaler Browserlink zum geprüften Sicherungs-Endpunkt. Der Browser verarbeitet den Anhang nativ in einem neuen Tab statt nach einem asynchronen `fetch()` einen künstlichen Download-Klick auszulösen. Das manuelle Speicherhäkchen und die serverseitige Sperre vor Update/Rückkehr bleiben bestehen.
- Testgerät vorher: COM13, USB-Seriell CH340, `192.168.0.154`, Firmware 1.8.10, Controller-ID-Suffix `842178`, Master. OTA-Update auf 1.8.11 über `/update`: PASS. Nachher dieselbe Controller-ID, IP und Master-Rolle.
- Konfigurationserhalt: PASS; vor dem OTA gespeicherte lokale Sicherung liegt privat außerhalb des Repositorys. Danach stimmen lokale Konfiguration, gemeinsame Konfiguration und WLAN-Zustand beim vollständigen JSON-Vergleich überein. Die produktiven ESPs wurden nicht verändert.
- Live-HTTP: `/update` liefert HTTP 200, Version 1.8.11 und den neuen direkten Link `/api/update-backup`. Der Sicherungs-Endpunkt lieferte HTTP 200, Format `AntennaControllerSafetyBackupV2` und `Content-Disposition` als Anhang; Inhalt und Zuordnung zu 1.8.11 / Controller-IP stimmen.
- Browser-Klick auf den sichtbaren Sicherungslink im Benutzerbrowser: `OFFEN` bis zum angekündigten manuellen Klicktest. Der neue Link und der Endpunkt sind am ESP geprüft; die Browseroberfläche selbst wurde nicht automatisiert angeklickt.
- GitHub-Veröffentlichung: PASS; `v1.8.11` ist neueste stabile Veröffentlichung mit genau einem Produkt-Asset. Anonymer Direktdownload `/releases/latest/download/firmware-esp32dev.bin`: 1.507.328 Byte; SHA-256 stimmt vollständig mit dem lokalen Paket überein.
- Installierte Hilfsprogramme: PlatformIO Core 6.2.0 steht sowohl dem Benutzer als auch dem in der Projekt-Buildkette verwendeten Environment zur Verfügung; die vorherige Core-Versionswarnung ist behoben.
- Frühere separate Prüfung: Das Diagnoseformular wurde am 10.10.2026 mit Firmware 1.8.10 Ende zu Ende versendet und der Eingang bestätigt. Siehe Versionshistorie; in diesem Update nicht erneut ausgeführt.

Der am 10.10.2026 nachgetragene Meldeformular-Test wurde separat als Dokumentationscommit `79b9d9340fb0bdaeac265a3bf6815cf1454bae85` auf GitHub `main` veröffentlicht; Firmware und Geräte blieben dabei unverändert.

Ältere Abschnitte sind Versionshistorie. Ein dortiger PASS gilt nur für die ausdrücklich genannte Version. Build- oder HTTP-Readback belegt nicht automatisch den Löschstatus jeder einzelnen LittleFS-Datei.

Dieses Protokoll unterscheidet Quelltext- und Buildprüfungen von Prüfungen an echten Geräten. Ein erfolgreicher Build allein beweist keine OTA-Funktion.
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

## Änderungen in 1.8.5

- Sicherungsdateien erhalten einen Dateinamen mit Controller-ID und aktueller IP-Adresse. Dieselbe Zuordnung wird im JSON-Inhalt mitgeführt.
- Sicherungsdialoge erklären direkt am Download: Die Sicherung umfasst den ESP, dessen eigene Oberfläche geöffnet wurde, plus gemeinsame Anlagenkonfiguration und dessen WLANs. Für eine vollständige Verbundsicherung wird jeder ESP einzeln auf seiner eigenen Adresse gesichert.
- Die Bedienoberfläche behält die gespeicherten 12×6-Rasterpositionen über Bildschirmgrößen hinweg bei. Responsive Regeln skalieren die Rasterfläche und Beschriftungen, ohne Karten automatisch neu anzuordnen.
- Vier sichtbar angelegte Reihen wie in der gespeicherten Benutzeranordnung bleiben erhalten; vier Reihen werden nicht global erzwungen.
- Vollständiges OTA-Paket enthält Firmware und alle geänderten Webdateien in einer Datei. LittleFS mit den vorhandenen Einstellungen wird beim OTA nicht geschrieben.
- Es gibt nur noch den Build `esp32dev` und eine OTA-Datei. Die frühere Jungfrau-Variante wurde aus dem aktiven Build und den aktuellen Download-Anweisungen entfernt.
- Die Schaltfläche „Neueste Firmware direkt herunterladen“ verwendet GitHub Releases/latest/download mit dem festen einzigen Assetnamen. Sie öffnet keine Release-Auswahlseite.

## Buildstatus 1.8.5

| Prüfung | Ergebnis |
|---|---|
| PlatformIO `esp32dev` plus vollständiges OTA-Paket | PASS; Firmware 1.449.237 / 1.507.328 B, 52.960 / 327.680 B RAM; Paket 1.507.328 B mit Firmware 1.455.808 B, Webdateien 48.537 B und 2.947 B Reserve |
| SHA-256 `firmware-esp32dev.bin` | `f789f44ef19942126bb72bf9252c8443f3f72f686e79e4a3505be408a36a8af5` |
| ESP32-Imageprüfung | PASS; `esptool` 4.9.0 meldet gültige Image-Prüfsumme und SHA-256 für die vollständige 1.507.328-Byte-Datei |
| GitHub-Release `v1.8.5` | PASS; öffentlich, stabil, als neueste Version markiert und enthält genau `firmware-esp32dev.bin` |
| GitHub-Direktdownload `/releases/latest/download/firmware-esp32dev.bin` | PASS; 1.507.328 Byte, SHA-256 stimmt vollständig mit dem lokalen Paket überein |
| Firmware-Quellcommit | PASS; `294fb406805eddbbde00396102ea53d3d507ed94` |
| OTA auf echtem ESP | OFFEN; Installation muss am Gerät noch ausgeführt und geprüft werden |
| Positionsgleichheit und Scrollfreiheit auf echten Handy-, Laptop- und 4K-Ansichten | OFFEN; noch keine Sichtprüfung auf diesen Geräten |

Der Release wurde am 09.10.2026 erstellt. Die erfolgreiche öffentliche Direktdownloadprüfung belegt, dass GitHub die erwartete Binärdatei ausliefert; sie ersetzt keinen OTA-Lauf am ESP.

## Änderungen in 1.8.6

- Die Bedienseite trennt vor dem Einschalten eines gewählten Signalwegs alle anderen aktiven oder teilweise aktiven Wege. Ein bereits allein aktiver Weg lässt sich durch erneutes Anklicken trennen.
- Die grünen Markierungen zeigen den bestätigten Relaiszustand auch dann an, wenn eine alte Konfiguration mehrere Wege eingeschaltet hat, damit ein Konflikt sichtbar bleibt und bereinigt werden kann.
- Die ESP-seitige Route-API erlaubt keine neue Verbindung, solange ein anderer Weg noch Relais eingeschaltet hat. Sie verweigert das Schalten außerdem, wenn ein für andere Wege zuständiges Verbundgerät nicht zuverlässig erreichbar ist und sein Zustand daher nicht sicher geprüft werden kann.
- Es bleibt bei einer vollständigen ESP32-DevKit-Firmwaredatei. Das normale OTA schreibt nicht in LittleFS.

## Buildstatus 1.8.6

| Prüfung | Ergebnis |
|---|---|
| PlatformIO `esp32dev` Firmware | PASS; 1.450.629 / 1.507.328 Byte Flash (96,2 %), 52.960 / 327.680 Byte RAM |
| Vollständiges OTA-Paket `firmware-esp32dev.bin` | PASS; 1.507.328 Byte, Firmware 1.457.200 Byte, gebündelte Webdateien 48.640 Byte, 1.452 Byte Reserve |
| ESP32-Imageprüfung | PASS; esptool 4.9.0, Prüfsumme und Validierungshash gültig |
| SHA-256 `firmware-esp32dev.bin` | `e59837d962d002ef3851e9c28aa7c23f57472c6ea203583de881f9b77f2ec5b4` |
| GitHub-Release `v1.8.6` | PASS; öffentlich, stabil, neueste Version, genau ein Asset `firmware-esp32dev.bin` |
| Öffentlicher Direktdownload `/releases/latest/download/firmware-esp32dev.bin` | PASS; 1.507.328 Byte, SHA-256 entspricht dem lokalen Paket |
| Firmware-Quellcommit | PASS; `1d790be805e09326629cbcbad21c2ff647669e6e` |
| Einzelverbindung am echten ESP, Wechsel zwischen unterschiedlichen Gruppen und erneutes Anklicken zum Trennen | OFFEN |
| Verbund: nicht erreichbarer Controller blockiert einen neuen Weg | OFFEN |

Das öffentliche Release wurde am 10.10.2026 erstellt. Der Direktdownload ist geprüft; der funktionale Schaltversuch auf echter Hardware steht noch aus.

| Prüfung | Ergebnis |
|---|---|
| GitHub-Hauptzweig enthält v1.8.4 | PASS; Release-Tag zeigt auf den veröffentlichten Stand |
| GitHub-Release v1.8.4 mit beiden damaligen OTA-Dateien | PASS; historische Release-Fassung |

## Veröffentlichung 1.8.7

- Diagnose ergänzt ein freiwilliges Formular für Fehler und Wünsche. Vor dem Senden zeigt der externe HTTPS-Endpunkt eine Vorschau mit zweiter Bestätigung; bei Nichterreichbarkeit bleibt der lokale JSON-Download verfügbar.
- Der Bericht enthält nur vom Benutzer ausgefüllte Felder und die Firmware-Version. WLAN, Rufzeichen, PLZ, GPIO-/Anlagenkonfiguration, IP-/MAC-Adresse und Sicherungen werden nicht automatisch angehängt.
- Die Fußzeile ergänzt Links zu Lizenz-/Sicherheitshinweisen und freiwilliger PayPal-Unterstützung. Versions- und Cachekennungen stehen auf 1.8.7.
- Das öffentliche Update bleibt ein einzelnes vollständiges ESP32-DevKit-OTA-Image. Die separate Konfigurations-/WLAN-Partition wird durch dieses OTA nicht überschrieben.

| Prüfung | Ergebnis |
|---|---|
| JavaScript-Syntax `data/app.js` | PASS; `node --check` |
| PlatformIO `esp32dev` | PASS; RAM 52.960 / 327.680 Byte (16,2 %), Firmware-ELF 1.451.269 / 1.507.328 Byte (96,3 %) |
| Vollständiges OTA-Paket | PASS; 1.507.328 Byte, Roh-Firmware 1.457.840 Byte, Brotli-Weboberfläche 48.652 Byte, 800 Byte Reserve |
| ESP32-Image | PASS; esptool 4.9.0, gültige Prüfsumme und Validierungshash für genau dieses Paket-SHA |
| SHA-256 `firmware-esp32dev.bin` | `9dd2658fb53ac48e4e2870f459be088afdc4e609ad0f3953ffeb9eacbd8258f0` |
| Mail-Endpunkt auf do1anb.de | OFFEN; Live-Abruf liefert HTTP 404. Die Upload-Datei enthält ausschließlich `index.php`; Serverkonfiguration und echter Versandtest fehlen noch. |
| Berichtformular im echten Browser und SMTP-Zustellung | OFFEN; Hosting-Endpunkt muss zuerst eingerichtet werden. |
| OTA und Schaltlogik an den echten ESPs | OFFEN; noch kein Gerät mit diesem Arbeitsstand aktualisiert oder geschaltet. |
| Handy-, Laptop- und 4K-Darstellung | OFFEN; keine Sichtprüfung mit diesen realen Bildschirmgrößen durchgeführt. |
| GitHub-Release `v1.8.7` | PASS; öffentlich, stabil und als neueste Version markiert; genau ein Asset `firmware-esp32dev.bin` |
| Öffentlicher Direktdownload `/releases/latest/download/firmware-esp32dev.bin` | PASS; 1.507.328 Byte, SHA-256 stimmt mit lokalem Paket überein |

Der lokale Build und die öffentliche Downloadprüfung wurden am 10.10.2026 ausgeführt. Der Firmware-Quellcommit ist `996f159b94ab92aedfdbec9b533d436d96d1350b`. Wegen nur 800 Byte freiem Platz im festen OTA-Slot muss jede weitere Firmware- oder Oberflächenänderung erneut gegen denselben Paketbauer geprüft werden. Die hier aufgeführten offenen Geräte- und Hostingprüfungen dürfen nicht als bestanden dargestellt werden.

## Veröffentlichung 1.8.8 – Fehlerkorrektur im bestehenden Wiederherstellungsablauf

Die Wiederherstellung des zuletzt gespeicherten Schaltzustands nach einem Neustart war bereits in älteren Firmwareversionen vorhanden. 1.8.8 führt diese Funktion nicht neu ein. Die Codeprüfung fand einen Sonderfall in diesem bestehenden Ablauf: War die TX-Sperre beim Start aktiv, löschte die Firmware die gespeicherte Auswahl, bevor das durch die Sperre blockierte Einschalten der Relais erfolgreich sein konnte. 1.8.8 behebt genau diesen Verlustpfad: Die Auswahl bleibt gespeichert und die Wiederherstellung wird nach Ende der TX-Sperre erneut versucht. Auch bei anderen fehlgeschlagenen Wiederherstellungen bleibt die gespeicherte Auswahl erhalten.

| Prüfung | Ergebnis |
|---|---|
| `node --check data/app.js` | PASS |
| PlatformIO `esp32dev` | PASS; RAM 52.968 / 327.680 Byte (16,2 %), Firmware-ELF 1.451.349 / 1.507.328 Byte (96,3 %) |
| Vollständiges OTA-Paket | PASS; 1.507.328 Byte, Roh-Firmware 1.457.920 Byte, Brotli-Weboberfläche 48.655 Byte, 717 Byte Reserve |
| SHA-256 `firmware-esp32dev.bin` | `786396a295bd8e63bb4524748e18fb31d2472cf9132ec5eee191858f84defec1` |
| Neustart mit aktiver TX-Sperre und spätere Wiederherstellung | OFFEN; noch nicht am ESP geprüft |
| GitHub-Release `v1.8.8` | PASS; öffentlich, stabil, neueste Version; genau ein Binär-Asset `firmware-esp32dev.bin`, 1.507.328 Byte |
| Release-Text auf GitHub | PASS; am 10.10.2026 präzisiert: Die allgemeine Zustandswiederherstellung bestand bereits vorher; 1.8.8 korrigiert nur den Sonderfall mit aktiver TX-Sperre. Firmwaredatei und Prüfsumme blieben unverändert. Routine- und Detailprüfungen stehen im Prüfprotokoll, nicht als angeblich neue Funktion im Updatehinweis. |
| GitHub-Tag und `main` | PASS; Tag `v1.8.8` und `main` zeigen beide auf Quellcommit `f546e07a1ae80dcc686872e313647761acd95b01` |
| Öffentlicher Direktdownload `/releases/latest/download/firmware-esp32dev.bin` | PASS; HTTP leitet auf `/releases/download/v1.8.8/firmware-esp32dev.bin` weiter und liefert 1.507.328 Byte mit passendem SHA-256 `786396a295bd8e63bb4524748e18fb31d2472cf9132ec5eee191858f84defec1` |
| Mail-Endpunkt auf do1anb.de | PASS; Live-Health HTTP 200 mit `{"ready":true}`; direkter Abruf von `mail-config.php` bleibt leer HTTP 403. Ein zuerst bereitgestelltes privates Paket enthielt ein falsches SMTP-Passwort und wurde ersetzt. |
| SMTP-Berichtversand und Empfang | PASS mit Einschränkung; zwei Testberichte wurden angenommen und empfangen. GMX sortierte sie in Spam ein. Im zweiten Nachrichtenkopf bestanden SPF, DKIM, DMARC und IP-Reverse-Prüfung; der Betreiber akzeptiert die Spam-Einstufung für seinen alleinigen Empfänger. |
| DMARC-DNS | PASS; seit dem 10.10.2026 ist `_dmarc.do1anb.de` mit `v=DMARC1; p=none;` öffentlich sichtbar. |
| Berichtformular direkt aus der Firmware-Weboberfläche | OFFEN; vollständiger Klickpfad über ein reales ESP-Gerät ist noch nicht geprüft. |
| Sonstige echte ESP-/Displayprüfungen | OFFEN; siehe oben |

Der Paketbauer hat die vollständige Firmware und Weboberfläche erneut in den festen OTA-Slot gepackt. Es bleiben 717 Byte Reserve, deshalb sind weitere Funktionen nur nach erneutem vollständigem Paketbuild zulässig. Die öffentliche Release- und Direktdownloadprüfung erfolgte am 10.10.2026.

## Flashplatz-Analyse v1.8.8 – statische Bestandsaufnahme vom 10.10.2026

Es wurde kein neuer Build gestartet und kein Firmwarecode verändert. Untersucht wurden das vorhandene `firmware.bin`, der Linker-Map-/ELF-Stand des v1.8.8-Pakets, die Partitionstabelle und die Buildflags.

| Bestandteil | Größe / Stand |
|---|---:|
| Firmware-Image | 1.457.920 Byte |
| fester OTA-App-Slot (`0x170000`) | 1.507.328 Byte |
| zusätzlich gebündelte, minifizierte und Brotli-komprimierte Webdateien | 48.655 Byte |
| Paket-Endmarkierung | 36 Byte |
| freie Reserve im vollständigen OTA-Paket | **717 Byte** |

Die Kompilierung nutzt bereits `-Os`, getrennte Funktions-/Datensektionen und Linker-Garbage-Collection. Die Linkerflags setzen jedoch ausdrücklich `-fno-lto`; Link-Time-Optimierung ist daher der aussichtsreichste erste A/B-Versuch, ohne Funktionen zu entfernen. Vor einer Übernahme müssen unverändertes und LTO-Build mit identischer Toolchain verglichen, das vollständige OTA-Paket geprüft und die betroffenen Abläufe regressionsgeprüft werden. Eine Ersparnis ist nicht zugesichert.

Die Map zeigt außerdem größere Bereiche für HTTP-/Konfigurationslogik und C-Bibliotheksformatierung. Sechs projektinterne `snprintf`-Aufrufe erzeugen festbreite Hex-/MAC-Kennungen; deren Ersatz durch kleine spezialisierte Formatter könnte die allgemeine Formatierungsbibliothek verkleinern, sofern keine Framework- oder Bibliotheksaufrufe dieselben Routinen weiterhin benötigen. Das muss über einen A/B-Linker-Map-Vergleich nachgewiesen werden. Die vorhandene Webminifizierung und Brotli-Qualität 11 sind bereits aktiv; hier ist kein ungenutzter Minifizierungsschritt ersichtlich.

**Reihenfolge für weitere Arbeit:** zuerst LTO isoliert messen; danach die wenigen Hex-Formatter isoliert prüfen. Keine Funktion und kein Bedienkomfort wird für Platzgewinn entfernt. Eine Änderung der Partitionstabelle wäre kein risikoloser Optimierungsschritt: Sie kann eine vollständige Neuinstallation und Migration des LittleFS mit allen lokalen Einstellungen erfordern und bleibt deshalb vorerst außen vor.

## Isolierter LTO-A/B-Vergleich vom 10.10.2026

Beide Builds wurden aus demselben sauberen Quellcommit `d06587e3f77460c5d4987192c6ef38b84a16a5a2` in getrennten temporären Arbeitsbäumen erzeugt. Der LTO-Versuch änderte nur Compiler-/Linkerflags; es wurden keine Produktdateien, Geräte oder gespeicherten Konfigurationen geändert. Der endgültige LTO-Linkeraufruf wurde geprüft: `-flto` ist enthalten und `-fno-lto` nicht. Ein vorheriger Versuch, der nur `-flto` beim Kompilieren setzte, wurde deshalb nicht als LTO-Ergebnis gewertet.

| Messwert | Ohne LTO | Mit LTO | Änderung |
|---|---:|---:|---:|
| Firmwaredatei | 1.457.280 Byte | 1.363.376 Byte | **−93.904 Byte (−6,44 %)** |
| Vom Linker belegter Programmspeicher | 1.450.709 Byte | 1.356.805 Byte | −93.904 Byte |
| Statischer RAM laut Build | 52.968 Byte | 52.352 Byte | −616 Byte |
| Brotli-Weboberfläche im OTA-Paket | 48.648 Byte | 48.648 Byte | unverändert |
| Reserve im vollständigen OTA-Slot | 1.364 Byte | 95.268 Byte | **+93.904 Byte** |

Werkzeugstand: PlatformIO `espressif32 6.12.0`, Arduino-ESP32 `3.20017.241212+sha.dcc1105b`, Xtensa-GCC `8.4.0+2021r2-patch5`, esptool `4.9.0`. Beide vollständigen OTA-Pakete bestanden den Paketbau einschließlich Größen-, Brotli-Dekompressions-, Footer- und CRC-Prüfung (`PACKAGE PASS`); beide Pakete sind jeweils 1.507.328 Byte groß. SHA-256 ohne LTO: `8B74BF22D270E4F609D8E3CC9A4C72C8DCAE01DEF7249587C14A0C3CE0899345`; mit LTO: `0DD6BA7BEE3ABA46F25512905C2DCF980407488DBDF2A1FEBC7699FB7B68D0D5`.

**Bewertung:** Der Platzgewinn ist deutlich und entfernt keine Funktionen. LTO bleibt ein isolierter Kandidat und ist noch nicht in der Produktkonfiguration oder einem Release aktiviert. Der gezielte Start- und Webabruf wurde anschließend auf einem Test-ESP geprüft; vollständige Relais-, Signalweg-, TX-Sperren-, Wiederherstellungs- und Verbundprüfungen bleiben `OFFEN`. Der ältere Wert von 717 Byte Reserve oben bezieht sich auf das damals vorliegende Buildartefakt; für den hier frisch aus Commit `d06587e` gebauten unveränderten Stand beträgt die Reserve ohne LTO 1.364 Byte.

### Test-ESP mit LTO-Abbild am 10.10.2026

| Prüfung | Ergebnis |
|---|---|
| Gerät am USB-Port COM13 | PASS; ESP32-D0WD-V3, MAC-Suffix `E2FD28`; die Gerätekennung stimmte mit der Routeranzeige des Testgeräts überein |
| Sicherung vor dem Schreiben | PASS; vollständiger 4-MB-Flashauszug, 4.194.304 Byte, lokal unter `backups/device-flash/` (ignoriert, nicht veröffentlichen); SHA-256 `4FD77B5D366834D5153CFD6E8AD519A431D51581B79D821EE68CD6CADC33407E` |
| Flashaufbau vor dem Schreiben | PASS; Partitionstabelle ausgelesen; aktiver Slot `app0` bei `0x10000`, Größe `0x170000`; separater LittleFS-Bereich ab `0x2F0000` |
| LTO-OTA-Paket | PASS; 1.507.328 Byte; SHA-256 `CB809CAC1FBC8895E1F7E94165B0650AF4C68A7CED3AAB299906271279B94981`; vollständiger Paketbauer-Lauf erfolgreich |
| Schreiben | PASS; ausschließlich `app0` über USB bei `0x10000` geschrieben und vom esptool vollständig zurückgelesen/verifiziert. Bootloader, Partitionstabelle, OTA-Auswahl, NVS und LittleFS wurden nicht beschrieben. |
| Neustart und Firmware-Readback | PASS; `/api/snapshot` antwortete HTTP 200 und meldete Firmware `1.8.8`, API-Version 6 und die erwartete Gerätekennung |
| Konfigurationserhalt | PASS; Geräte-/Netzkonfiguration blieb auf den nicht beschriebenen NVS-/LittleFS-Partitionen erhalten; im Readback waren die vorhandenen Geräte sichtbar. Es wurden keine Relais betätigt. |
| Weboberfläche | PASS; `/` HTTP 200 in 124 ms, `/app.js` HTTP 200 in 356 ms, `/responsive.css` HTTP 200 in 70 ms; die vom ESP gelieferte minifizierte `app.js` stimmt per SHA-256 exakt mit der LTO-Paketdatei überein. Messung über das lokale WLAN am 10.10.2026 |
| Gespeicherte Signalwege und Anlagenteile | PASS; Readback zeigt 16 Signalwege und 8 Layout-Elemente, passend zu 4 Funkgeräten × 4 Antennen und je 4 Geräte-/Antennenfeldern |
| Relais/Signalwege/TX-Sperre/Zustandswiederherstellung/Verbund | OFFEN; bei diesem Lauf nicht ausgelöst oder geprüft |

Die Sicherung enthält private Konfiguration und WLAN-Zugangsdaten und bleibt ausschließlich lokal. Das LTO-Abbild wurde nur auf diesem nichtproduktiven Test-ESP installiert; die Produkt-Buildkonfiguration, GitHub-Releases und die beiden Betriebsgeräte blieben unverändert.

**Buildumgebung repariert:** Der erste Buildversuch scheiterte an einer Windows-Zugriffssperre in `tool-esptoolpy` beim IntelHex-Import. Eine gezielte UAC-Neuinstallation des PlatformIO-Werkzeugs und Rechtekorrektur nur innerhalb dieses Paketordners behob das Problem. Danach ließ sich das Paket als normaler Benutzer erneut vollständig entfernen und sauber installieren; IntelHex-Import und unveränderter `esp32dev`-Build bestanden ohne Administratorrechte. Der fehlgeschlagene erste OTA-Paketaufruf verwendete außerdem einen Node-Pfad ohne mitgeliefertes `npx`; der erneute Aufruf mit dem installierten Node.js bestand.

## Firmware 1.8.9 – Browser-Komprimierung und Projektangaben

Am 10.10.2026 zeigte ein Screenshot der lokalen ESP-Weboberfläche unlesbare Zeichen statt der Webseite. Die HTTP-Antwort enthielt `Content-Encoding: br`, obwohl die Firmware die Anfrage nicht auf Brotli-Unterstützung prüfte. Der unveränderte Antwortkörper war gültiges Brotli und dekomprimierte zu der erwarteten HTML-Seite. Die vorherige Prüfung hatte nur HTTP 200 gewertet und fälschlich als erfolgreiche Seitenauslieferung dokumentiert.

Geändert für 1.8.9:

- Die ESP-Weboberfläche läuft derzeit über HTTP. Firmware und vollständige Weboberfläche sind gemeinsam im OTA-Abbild gebündelt. Bei gültigem Bündel werden keine getrennt gespeicherten LittleFS-Webdateien als Ersatz benutzt. Die Firmware liefert gzip bevorzugt aus und nutzt Brotli, wenn der Browser es anfordert und gzip nicht akzeptiert. Antworten kennzeichnen `Vary: Accept-Encoding`. Wenn kein unterstütztes Format angeboten wird, antwortet der ESP mit HTTP 406 und einer verständlichen Erklärung.
- Rufzeichen `DO1ANB` wurde im Footer ergänzt; Name, E-Mail, Lizenzlink und freiwilliger PayPal-Link bleiben erhalten.
- Das vorhandene Fehler-/Wunschformular steht am Anfang des Diagnose-Reiters.
- Versions- und Asset-Cachekennzeichnung wurde auf 1.8.9 angehoben.

**Prüfstatus:** Build und vollständiger OTA-Paketbau bestanden. PlatformIO `espressif32 6.12.0`, Arduino-ESP32 `3.20017.241212+sha.dcc1105b`, Xtensa-GCC `8.4.0+2021r2-patch5`, esptool `4.9.0`. RAM 52.376 / 327.680 Byte (16,0 %); App-Abbild 1.364.800 Byte; vollständiges OTA-Paket 1.507.328 Byte; verbleibender OTA-Slot-Puffer 35.616 Byte. Paket-SHA-256 `5FC0886ECE59A0FB7FB9BD09C587B837CFA0CD89D59CDEB4FEB3D1A9A7555C87`. Der Paketbauer prüfte JavaScript-Syntax und für alle Webdateien die gzip- und Brotli-Komprimierung samt Dekomprimierungsvergleich.

| Prüfung am nichtproduktiven Test-ESP | Ergebnis |
|---|---|
| OTA und Neustart | PASS; `/api/snapshot` meldet Firmware 1.8.9 und API 6 |
| Browser mit gzip-Anfrage | PASS; HTTP liefert `Content-Encoding: gzip`, entpackte Antwort ist gültiges HTML |
| Browser mit `Accept-Encoding: br, gzip` | PASS; gzip wird bevorzugt geliefert und korrekt dekodiert |
| Brotli allein angefordert | OFFEN; Paketvarianten sind beim Build dekomprimiert und verglichen worden |
| Browser ohne unterstützte Komprimierung | PASS; HTTP 406 statt einer möglicherweise veralteten LittleFS-Seite |
| Sichtbare Startseite | PASS; Browser zeigt „Antennensteuerung v1.8.9“ und lesbare Oberfläche |
| Footer und Diagnose | PASS; DO1ANB, Name, E-Mail, Lizenz-/PayPal-Links und Formular „Fehler oder Wunsch melden“ sichtbar |
| Gerätekonfiguration | PASS; gespeicherte Geräte, Signalwege und Layout wurden nach dem App-OTA ausgelesen; produktive ESPs wurden nicht verändert |
| Bericht absenden | NICHT AUSGEFÜHRT; der vollständige Klickpfad wurde sichtbar geprüft, aber es wurde keine Nachricht versendet |
| GitHub-Veröffentlichung | PASS; Release `v1.8.9` ist als neueste stabile Version markiert und enthält genau ein Produkt-OTA-Asset |
| Öffentlicher Direktdownload | PASS; `/releases/latest/download/firmware-esp32dev.bin`, 1.507.328 Byte; SHA-256 entspricht lokal `5FC0886ECE59A0FB7FB9BD09C587B837CFA0CD89D59CDEB4FEB3D1A9A7555C87` |

Eine geprüfte Sicherung der lokalen und gemeinsamen Konfiguration wurde vor dem Update ausschließlich lokal gespeichert. Die privaten Sicherungsdateien und WLAN-Zugangsdaten werden nicht veröffentlicht. Es gibt auf dem Gerät nur eine laufende Firmwareversion; Firmware und Weboberfläche werden zusammen aktualisiert. Die getrennte LittleFS-Partition bleibt erhalten, wird von dieser Firmware bei gültigem UI-Bündel aber nicht als ältere Webseiten-Version ausgeliefert.

## Schutzregeln für den manuellen Updateversuch

- Vor dem OTA die Sicherungsdatei auf den Computer herunterladen und speichern. Sie enthält WLAN-Kennwörter und muss privat bleiben.
- Nur die passende vollständige OTA-Datei hochladen. Die Datei enthält Firmware und aktuelle Oberfläche; die separate LittleFS-Partition mit gespeicherten Daten wird nicht überschrieben.
- Nach dem Neustart Version, Master-/Follower-Rolle, Rufzeichen, PLZ, WLAN, Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl und Peer-Status kontrollieren.
- Die Sicherungsdatei aufbewahren, bis alle Einstellungen nach dem Update geprüft sind.

## Datenschutz vor Veröffentlichung

Prüfe vor dem Upload Quelltext, Dokumente, Binärdateien, Anhänge und erreichbare Git-Historie auf Zugangsdaten, personenbezogene Angaben, KI-Dienste und Geräte-Konfigurationen. Veröffentlichte Prüfsummen müssen exakt zu den angehängten Release-Dateien gehören.


## Firmware 1.8.10 – Keine gemischten Oberflächenstände

Änderung am 10.10.2026:

- Das bisher getrennt aus LittleFS geladene `style.css` wird nun wie Startseite, JavaScript und `responsive.css` als Brotli-/gzip-Variante im vollständigen OTA-Paket gespeichert. Ein neuer Footer-Marker `ANTUIBR3` unterscheidet das Format von älteren Paketen. Die CRC32-Prüfung umfasst alle acht komprimierten Oberflächenrepräsentationen.
- Beim normalen Start löscht die Firmware nach erfolgreicher Marker-, Bereichs- und CRC-Prüfung ausschließlich `/index.html`, `/app.js`, `/responsive.css` und `/style.css`. Bei fehlendem oder ungültigem Paket wird nichts gelöscht und der LittleFS-Rückfall bleibt verfügbar.
- Ein Upload einzelner UI-Dateien wird abgewiesen, solange ein gültiges Paket läuft. Die URLs von CSS und JavaScript tragen den Cache-Schlüssel 1.8.10.
- WLAN, Gerätekonfiguration, Update-Sicherungen, `/setup.html`, Logo und Favicon sind nicht Ziel der Löschung. Der Paketbauer prüft die drei bewusst unveränderten Setup-/Markendateien anhand festgelegter SHA-256-Werte.

### Lokaler Build und Paket

| Prüfung | Ergebnis |
|---|---|
| Erster Buildlauf | FAIL; ein neu ergänzter Prüfsummenpfad verwendete zunächst eine nicht deklarierte Ende-Variable. Vor dem OTA korrigiert. |
| Zweiter Lauf `node tools/build-ota-package.mjs esp32dev` | PASS; PlatformIO `espressif32 6.12.0`, Arduino-ESP32 `3.20017.241212+sha.dcc1105b`, Xtensa-GCC `8.4.0+2021r2-patch5`, esptool `4.9.0`; RAM 52.384 / 327.680 Byte; Firmwareabbild 1.363.568 Byte. |
| Komprimierung | PASS; JavaScript-Syntax geprüft; alle vier Assets in Brotli und gzip komprimiert und zurückdekomprimiert mit bytegleichem Quellvergleich. |
| OTA-Slot | PASS; Gesamtpaket 1.507.328 Byte, Reserve 8.766 Byte nach Firmware, acht komprimierten Oberflächenrepräsentationen und dem 76-Byte-Footer. |
| Footer und CRC32 | PASS; Paketbauer prüfte Marker, Feldpositionen, Inhalt und CRC32. SHA-256 Gesamtpaket: `1C09D3202F6C2CA453BC960CB8EC3928C1945E09746B145995AFE6C1F66DB238`. |
| `git diff --check` und Manifest | PASS; vor Commit ausgeführt und aktualisiert. |

### OTA am nichtproduktiven ESP32

| Prüfung | Ergebnis |
|---|---|
| Zielidentität | PASS; vor OTA live bestätigt: ESP32-28FDE2842178, Master, Firmware 1.8.9, Adresse 192.168.0.154. |
| Sicherung | PASS; unmittelbar vor OTA heruntergeladen, Format `AntennaControllerSafetyBackupV2`, Zielkennung geprüft; lokale Datei unter `backups/config/`, von Git ignoriert. SHA-256 `A5B9FF3B4D130B95734698C2AD84D910FB7D3AC3D607BB30DE6668AFEEDC71D8`. |
| OTA | PASS; 1.507.328 Byte über die lokale Update-Seite gesendet, HTTP 200 bestätigte den Neustart. |
| Firmware-Readback | PASS; `/api/snapshot` meldet 1.8.10, API 6, dieselbe Controller-ID, Master-Rolle, Adresse und 8 Geräte. |
| Konfigurationserhalt | PASS; vollständiger JSON-Vergleich der lokalen Einstellungen, gemeinsamen Einstellungen und WLAN-Daten vor/nach OTA ist identisch. Nachher: 8 Geräte, ein gespeichertes WLAN. |
| vier HTTP-Hauptdateien | PASS; `/`, `/app.js`, `/responsive.css` und `/style.css` antworten HTTP 200 mit gzip. Dekomprimierte Inhalte stimmen exakt mit Quellhashes überein: HTML `3613AFC111BDF25EEFB9BE1F2419E0887D3DEF67C9A3BBBEE1D4FDC24B48D559`, JavaScript `E53F8553F7BC430705DB480FEDC146AAF990DF9DFC918E67E423451B4CAC0D5F`, responsive CSS `B287AB9E48B360AEB1FF344584662599F45094455A1C9738561CBC271C2830C3`, Basis-CSS `3CE062D7621FB246E174CCC273C0D517D0EA8DC45301C0ECB8DA1D2EE6DE1A78`. |
| UI-Einzeldatei-Upload | PASS; gezielter Uploadversuch mit `style.css` lieferte HTTP 409 mit der erwarteten Ablehnung; keine Datei wurde aktiviert. |
| Diagnoseformular und E-Mail-Eingang | PASS; echter Versand aus Firmware 1.8.10 am 10.10.2026, Eingang durch Betreiber bestätigt. Die empfangene Mail nennt die Formularangaben und Version 1.8.10 und bestätigt, dass keine Konfiguration angehängt war. Persönliche Testfelder sind nicht dokumentiert. |
| produktive ESPs | OFFEN / nicht angefasst. |
| LittleFS-Dateiliste nach Löschung | OFFEN; die Firmware bietet keinen Dateiliste-Readback. Paket-Auslieferung und Neustart wurden live geprüft, die physische Abwesenheit jedes Pfads kann nicht separat per API bestätigt werden. |
| GitHub-Veröffentlichung und Direktdownload | PASS; v1.8.10 ist öffentlich neueste stabile Version, genau ein Produkt-Asset; anonymer Direktdownload 1.507.328 Byte, SHA-256 entspricht dem lokalen Paket. |

### Nachtrag: Formularversand Ende zu Ende bestätigt

Am 10.10.2026 hat der Betreiber die Fehler-/Wunschmeldung direkt in der ESP-Weboberfläche ausgefüllt, den Inhalt in der HTTPS-Vorschau bestätigt und abgesendet. Der Betreiber meldete den Eingang im GMX-Postfach; der Screenshot zeigt die empfangene Nachricht vom konfigurierten Absender, die Firmware-Version 1.8.10 und den expliziten Satz, dass keine Konfiguration angehängt wurde. Damit ist die zuvor offene Prüfung „Bericht absenden“ für den erfolgreichen Normalfall `PASS`. Der Screenshot und die privaten Formularwerte werden nicht öffentlich abgelegt. Fehlerszenarien, Limits und der Download-Fallback bleiben `OFFEN`.

Die privaten Sicherungsdateien enthalten WLAN- und Gerätekonfiguration und bleiben lokal außerhalb der Veröffentlichung.

## Test-ESP: vollständiges Löschen und Jungfrau-Flash am 10.10.2026

- Auftrag: ausschließlich den ausgewiesenen Test-ESP vollständig löschen und die Standardfirmware ohne vorhandene Anlagen-/WLAN-Konfiguration flashen, damit der Betreiber die Sicherung anschließend manuell einspielen kann.
- Vorher-Sicherung: PASS; aktuelle `AntennaControllerSafetyBackupV2` mit Firmware 1.8.12, Controller-ID passend zum ESP, vollständige lokale/gemeinsame Konfiguration und WLAN-Zustand. Die persönliche Datei liegt außerhalb des Repositories auf dem Desktop des Betreibers; ihr Inhalt und ihre Geheimnisse sind nicht öffentlich dokumentiert.
- Identitätsprüfung vor Löschen: COM13 CH340, ESP32-D0WD-V3, MAC stimmt mit der im Sicherungsmetadatum enthaltenen Controller-ID überein. Vollständiges Flash-Erase mit esptool 5.3.1: PASS.
- Standardabbild anschließend per PlatformIO Core 6.2.0 aufgespielt: Firmware 1.8.12, 1.362.640 Byte. Neues LittleFS-Abbild aus den Produktdateien erstellt und geschrieben: 1.048.576 Byte; Flash-Readback-Hashprüfung der beiden Uploads PASS. WLAN-Zugangsdaten, NVS-Status und LittleFS-Konfiguration wurden durch das vollständige Löschen entfernt; frisches LittleFS enthält nur die Standard-Webdateien.
- Erststart: das Einrichtungs-WLAN `AntennaController-842178` wurde in der WLAN-Suche sichtbar. Damit ist der Jungfrau-Startweg aktiv. Netzwerk-Readback von Konfigurationslisten nach einer Verbindung mit dem Einrichtungs-WLAN: noch `OFFEN`.
- Konfigurationsimport wurde nicht durch Codex ausgelöst. Der Betreiber spielt die Sicherungsdatei selbst über **Konfigurieren → Sicherheit → Sicherung / Wiederherstellung → Konfiguration importieren** ein. Manuelle Schritte wurden in `ANLEITUNG.md` und `docs/FRESH-INSTALL.md` ergänzt.
- Keine produktiven ESPs verändert.

## Firmware 1.8.14: Stromtaster und GitHub-Release

- Release `v1.8.14`: PASS; veröffentlicht als neuestes stabiles Release auf GitHub, getaggt auf Commit `9e267c092dccc7419032925e3b050eb4dc829acc`. Releasehinweis: „Funktion: Eigenständige Stromtaster für Geräte.“
- Release-Asset: PASS; genau eine manuell installierbare Firmwaredatei `firmware-esp32dev.bin`, 1.507.328 Byte. SHA-256 `515822A52344D4EF8D1F5DBE280CABC2E2EEF94B1A16DA03EAA5BD70A0EB03FA`.
- Direktdownload: PASS; anonymer Download über `/releases/latest/download/firmware-esp32dev.bin` stimmt in Größe und SHA-256 mit dem gebauten Releasepaket überein.
- Test-ESP OTA: PASS; ESP32-28FDE2842178 auf 192.168.0.154 meldet Firmware 1.8.14 / API 7. Die vorhandene lokale, gemeinsame und WLAN-Konfiguration wurde vor und nach dem Firmware-OTA gesichert und verglichen.
- Strom-Kategorie: PASS; vier frei benennbare Anlagenteile „Strom Funkgerät 1“ bis „Strom Funkgerät 4“ mit unabhängigen Ein/Aus-Funktionen werden angezeigt. Ohne zugeordnetes Relais/GPIO sind die Taster sichtbar, aber gesperrt; die spätere Relaiszuordnung erfolgt durch den Betreiber.
- Bestandswege: PASS; die vorhandenen 16 Signalwege und sonstigen Konfigurationsfelder blieben beim Hinzufügen der Strom-Anlagenteile erhalten.
- Build/Paket: PASS; RAM 52.384 / 327.680 Byte, Firmwareabbild 1.364.976 Byte, OTA-Paket 1.507.328 Byte. UI Brotli 62.462 Byte, gzip 74.063 Byte, Paketreserve 5.751 Byte. Paket-SHA-256 wie oben.
