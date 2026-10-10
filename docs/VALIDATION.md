# Prüfprotokoll – Produktversion 1.8.8

## Aktueller veröffentlichter Stand

- Neueste stabile Firmware: **1.8.8**, GitHub-Tag `v1.8.8`.
- Firmware-Quellcommit und Release-Tag: `f546e07a1ae80dcc686872e313647761acd95b01`.
- Ein Produktbuild (`esp32dev`) und ein vollständiges OTA-Asset: `firmware-esp32dev.bin`.
- Öffentlicher Direktdownload, Paketgröße und SHA-256: siehe Abschnitt „Veröffentlichung 1.8.8“ weiter unten.
- SMTP-Endpunkt und Berichtversand: live bestätigt; GMX-Spamablage ist für den einzigen Empfänger akzeptiert. Der ESP-seitige Diagnose-Klickpfad bleibt offen.
- Reale Geräteprüfungen für Stromausfall-Wiederherstellung, Signalweg-/Verbundausschluss, Motorlauf und Bildschirmgrößen sind nicht pauschal bestanden; siehe jeweilige Zeilen mit `OFFEN`.

Die folgenden Abschnitte 1.8.1 bis 1.8.7 sind Versionshistorie. Ein älterer PASS gilt nur für den dort ausdrücklich genannten Quellstand und belegt nicht automatisch den Zustand von 1.8.8.

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

## Schutzregeln für den manuellen Updateversuch

- Vor dem OTA die Sicherungsdatei auf den Computer herunterladen und speichern. Sie enthält WLAN-Kennwörter und muss privat bleiben.
- Nur die passende vollständige OTA-Datei hochladen. Die Datei enthält Firmware und Oberfläche; sie aktualisiert nicht LittleFS.
- Nach dem Neustart Version, Master-/Follower-Rolle, Rufzeichen, PLZ, WLAN, Relais, Funktionen, Geräte, Signalwege, gespeicherte Auswahl und Peer-Status kontrollieren.
- Die Sicherungsdatei aufbewahren, bis alle Einstellungen nach dem Update geprüft sind.

## Datenschutz vor Veröffentlichung

Prüfe vor dem Upload Quelltext, Dokumente, Binärdateien, Anhänge und erreichbare Git-Historie auf Zugangsdaten, personenbezogene Angaben, KI-Dienste und Geräte-Konfigurationen. Veröffentlichte Prüfsummen müssen exakt zu den angehängten Release-Dateien gehören.

