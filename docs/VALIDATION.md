# Prüfprotokoll – Produktversion 1.8.9

## Aktueller veröffentlichter Stand

- Neueste stabile Firmware: **1.8.9**, GitHub-Tag `v1.8.9`.
- Ein Produktbuild (`esp32dev`) und ein vollständiges OTA-Asset: `firmware-esp32dev.bin`.
- Paketgröße: 1.507.328 Byte; SHA-256 `5FC0886ECE59A0FB7FB9BD09C587B837CFA0CD89D59CDEB4FEB3D1A9A7555C87`.
- Veröffentlichung: ein stabiles Release und ein einziges OTA-Asset; öffentlicher Direktdownload nach Upload zu prüfen.
- SMTP-Endpunkt und Berichtversand: live bestätigt; GMX-Spamablage ist für den einzigen Empfänger akzeptiert. Diagnoseformular auf dem ESP sichtbar geprüft; kein Bericht versendet.
- Reale Geräteprüfungen für Stromausfall-Wiederherstellung, Signalweg-/Verbundausschluss, Motorlauf und Bildschirmgrößen sind nicht pauschal bestanden; siehe jeweilige Zeilen mit `OFFEN`.

Die folgenden Abschnitte 1.8.1 bis 1.8.8 sind Versionshistorie. Ein älterer PASS gilt nur für den dort ausdrücklich genannten Quellstand und belegt nicht automatisch den Zustand von 1.8.9.

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
| GitHub-Veröffentlichung | OFFEN bis zum erfolgreichen Upload und anonymen Direktdownload |

Eine geprüfte Sicherung der lokalen und gemeinsamen Konfiguration wurde vor dem Update ausschließlich lokal gespeichert. Die privaten Sicherungsdateien und WLAN-Zugangsdaten werden nicht veröffentlicht. Es gibt auf dem Gerät nur eine laufende Firmwareversion; Firmware und Weboberfläche werden zusammen aktualisiert. Die getrennte LittleFS-Partition bleibt erhalten, wird von dieser Firmware bei gültigem UI-Bündel aber nicht als ältere Webseiten-Version ausgeliefert.

## Schutzregeln für den manuellen Updateversuch

- Vor dem OTA die Sicherungsdatei auf den Computer herunterladen und speichern. Sie enthält WLAN-Kennwörter und muss privat bleiben.
- Nur die passende vollständige OTA-Datei hochladen. Die Datei enthält Firmware und aktuelle Oberfläche; die separate LittleFS-Partition mit gespeicherten Daten wird nicht überschrieben.
- Nach dem Neustart Version, Master-/Follower-Rolle, Rufzeichen, PLZ, WLAN, Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl und Peer-Status kontrollieren.
- Die Sicherungsdatei aufbewahren, bis alle Einstellungen nach dem Update geprüft sind.

## Datenschutz vor Veröffentlichung

Prüfe vor dem Upload Quelltext, Dokumente, Binärdateien, Anhänge und erreichbare Git-Historie auf Zugangsdaten, personenbezogene Angaben, KI-Dienste und Geräte-Konfigurationen. Veröffentlichte Prüfsummen müssen exakt zu den angehängten Release-Dateien gehören.

