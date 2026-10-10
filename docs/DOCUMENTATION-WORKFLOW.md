# Verbindliche Arbeits-, Dokumentations- und Veröffentlichungsregeln

Diese Regeln gelten für Firmware, Weboberfläche, Server-Endpunkt, GitHub-Release und lokale Produktkopie. Eine Änderung gilt erst als abgeschlossen, wenn der Code, die passende Dokumentation und die tatsächlich ausgeführten Prüfungen denselben Stand beschreiben.

## Verbindliche Quellen und Reihenfolge

1. **Arbeitsquelle:** der ausdrücklich benannte Git-Arbeitsstand. Vor Änderungen Branch, Commit und Arbeitsbaum prüfen; vorhandene Änderungen anderer Arbeiten erhalten.
2. **Veröffentlichte Quelle:** `main`, Release-Tag und Release-Asset. Nie aus einem nicht geprüften oder abweichenden Arbeitsstand veröffentlichen.
3. **Gerätewahrheit:** nur aktuelle Messung oder Readback vom konkreten ESP. Ein Quellcode, Build, Screenshot, früheres Protokoll oder Erfolg eines anderen Geräts beweist keinen aktuellen Gerätezustand.
4. **Prüfprotokoll:** `docs/VALIDATION.md` ist der vollständige Nachweis mit Datum, Version und Commit. Neueste Erkenntnisse stehen im aktuellen Abschnitt; ältere Abschnitte bleiben als Historie gekennzeichnet und werden nicht stillschweigend als aktueller Stand ausgegeben.
5. **Produktstatus:** `README.md` beschreibt nur den derzeit veröffentlichten Stand. Geplante oder noch nicht getestete Funktionen müssen ausdrücklich als geplant beziehungsweise offen gekennzeichnet sein.

Wenn zwei Quellen einander widersprechen, ist die Arbeit **nicht freigabefähig**. Erst Ursache und tatsächlichen Stand klären, dann alle betroffenen Stellen gemeinsam berichtigen. Nicht einfach den bequemsten Eintrag übernehmen.

## Dokumentationspflicht bei jeder Änderung

Vor dem Bearbeiten eine kurze Änderungsübersicht anlegen: Ziel, betroffene Dateien und Funktionen, Risiken für gespeicherte Konfiguration sowie geplante Prüfungen. Nach der Änderung muss der Eintrag mindestens Folgendes enthalten:

- Datum, Produktversion und betroffener Quellcommit (oder klar „noch nicht veröffentlicht“)
- jede geänderte Datei beziehungsweise Komponente und den konkreten Zweck der Änderung
- Auswirkungen und ausdrücklich **nicht** veränderte Bereiche, besonders WLAN, Geräte-/Relaiskonfiguration, Layout, OTA und LittleFS
- ausgeführte Prüfungen mit genauer Methode und Ergebnis `PASS`, `FAIL` oder `OFFEN`
- bekannte Einschränkungen, Fehlversuche und erforderliche nächste Schritte
- bei Builds: Umgebung, Werkzeugversionen sowie tatsächliche RAM-/Flashwerte und Paketgröße
- bei Veröffentlichungen: Tag, Release-URL, Assetname, Bytezahl und SHA-256
- bei Geräteeingriffen: Gerätekennung in datenschutzgerechter Form, Rolle, Version vorher/nachher, Sicherungsstatus, tatsächlich verglichene Konfigurationsbereiche und Ergebnis
- bei externen Diensten: geprüfter Endpunkt, Zeit, HTTP-/SMTP-Ergebnis, Datenschutzgrenzen und noch nicht geprüfte Fehlerfälle

„Erledigt“, „funktioniert“ oder „getestet“ ohne diese konkreten Angaben ist kein ausreichender Eintrag. Nicht ausgeführte Prüfungen bleiben `OFFEN`; Vermutungen werden als Vermutung markiert. Fehler und fehlgeschlagene Versuche werden nicht aus der Historie entfernt, sondern mit der späteren Korrektur verknüpft.

## Dokumente, die gemeinsam aktuell gehalten werden

| Änderung betrifft | Pflichtdokumentation |
|---|---|
| Bedienung oder sichtbare Funktion | `README.md`, `ANLEITUNG.md`, `docs/VALIDATION.md`; bei Schnittstellen zusätzlich `docs/API.md` |
| Architektur, Speicher, Rollen oder Verbund | `docs/ARCHITECTURE.md`, `docs/API.md`, `docs/VALIDATION.md` und betroffene Anwenderanleitung |
| Konfiguration, Erstinstallation, Reset oder Wiederherstellung | `README.md`, `ANLEITUNG.md`, `docs/FRESH-INSTALL.md`, `docs/VALIDATION.md` |
| Update, OTA, Sicherung, Partition oder Release | `README.md`, `ANLEITUNG.md`, `docs/ARCHITECTURE.md`, `docs/VALIDATION.md`, Paketmanifest und Releasehinweise |
| Diagnoseformular, Maildienst oder Datenschutz | `docs/REPORT-ENDPOINT.md`, `server/report-endpoint/README-UPLOAD.md`, `docs/LEGAL-NOTICES.md`, `README.md` und Prüfprotokoll |
| Lizenz, Copyright oder Drittanbieter | `LICENSE`, `THIRD-PARTY-NOTICES.md`, `docs/LEGAL-NOTICES.md`, `README.md` |
| geplante Windows-App | `docs/WINDOWS-APP-PLAN.md`; Planung niemals als implementiert darstellen |
| beliebige Änderung | dieses Prüfprotokoll; weitere Dokumente aus der Tabelle, wenn sie betroffen sind |

Nicht betroffene Dokumente müssen nicht künstlich geändert werden. Die Änderungsübersicht muss aber zeigen, dass die Tabelle geprüft wurde und warum andere Bereiche unberührt bleiben.

## Pflicht-Gates vor dem Zusammenführen

1. **Ausgangslage:** Arbeitsbaum, Branch, Commit und vorhandene Nutzeränderungen erfassen. Keine ungesicherten Nutzeränderungen überschreiben.
2. **Umfang:** betroffene Funktionen und Abhängigkeiten im Quellcode verfolgen; Konfigurationsmigration, OTA-Verhalten, Einzelgerät/Verbund und Sicherheitsgrenzen prüfen, soweit betroffen.
3. **Änderung:** nur den nötigen Umfang ändern. Keine Zugangsdaten, privaten Konfigurationen oder unaufgeforderten Geräteänderungen einbauen.
4. **Dokumentation:** alle einschlägigen Dokumente aus der Tabelle aktualisieren; Versionsnummern, Dateinamen, Aussagen und Status übergreifend abgleichen.
5. **Prüfung:** für jede Änderung die geeigneten Prüfungen durchführen. Keine neue Prüfung als bestanden eintragen, die nicht tatsächlich gelaufen ist. Build allein ist kein Funktions-, Geräte- oder Sicherheitsnachweis.
6. **Konsistenz:** Diff und Manifest prüfen, auf widersprüchliche Versionsstände, veraltete „aktuell“-Aussagen, TODOs, fehlende Dateien und private Daten achten. `git diff --check` muss sauber sein.
7. **Freigabeentscheidung:** alle Release-Blocker ausräumen oder die Veröffentlichung ausdrücklich stoppen. Offene, aber nicht releasekritische Geräteprüfungen sichtbar als offen dokumentieren; nie als PASS tarnen.

## Build- und OTA-Regeln

- Produktbuild ist ausschließlich `esp32dev`. Keine Jungfrau-/Parallelvariante als zweite Produktversion veröffentlichen.
- Nach einer Firmware- oder Webänderung die betroffene PlatformIO-Umgebung bauen und tatsächliche Werte dokumentieren.
- Für das einzige vollständige OTA-Asset `node tools/build-ota-package.mjs esp32dev` verwenden. Es muss Firmware und sämtliche geänderten Webdateien enthalten. Einzelne UI-Dateien sind kein vollständiges Update.
- Paketformat, Slotgröße und Reserve prüfen. Paketbauer-Warnungen oder zu kleine Reserve sind vor Veröffentlichung zu behandeln; niemals Paketinhalt stillschweigend weglassen.
- Normales OTA darf LittleFS und die gespeicherte Geräte-/WLAN-Konfiguration nicht überschreiben. Erstinstallation und Factory-Reset sind getrennte Vorgänge und müssen als löschend beschrieben werden.
- Vor einer realen Geräteaktualisierung zuerst den expliziten Auftrag, die richtige Geräteidentität und die vorhandene Sicherung prüfen. Nach dem Update Version, Rolle, Verbund, Konfiguration und Erreichbarkeit per Readback kontrollieren. Niemals die lokale Konfiguration durch eine jungfräuliche Vorlage ersetzen.

## Sicherung und Synchronisierung der lokalen Produktkopie

- Vor dem Kopieren in `E:\ESP32-Projekt-Antennenschalter\AntennaController` die zu überschreibenden Dateien mit Zeitstempel außerhalb des Repositories sichern. Nutzerdateien, Sicherungen, ignorierte Builddateien und lokale Zugangsdaten bleiben erhalten.
- Nur die im Produktmanifest aufgeführten Projektdateien übertragen. Danach Quelle und Ziel dateiweise per SHA-256 vergleichen und fehlende, zusätzliche oder abweichende Dateien ausdrücklich auflisten.
- Manifest nach jeder Änderung an erfassten Dateien neu erzeugen und gegen beide Kopien prüfen. Eine erfolgreiche Kopie ist erst nach dem Vergleich bestätigt.
- Projektstatus und Übergabehinweise müssen denselben geprüften Commit und dieselben offenen Aufgaben nennen. Veraltete Projektstatusdateien sind vor Übergabe zu berichtigen.

## Veröffentlichung auf GitHub

1. Quelländerungen und Dokumentation auf demselben geprüften Stand vollständig committen und in `main` übernehmen.
2. Versionsnummer in Firmware, Cachekennungen, README, Prüfprotokoll, Paket und Release abgleichen.
3. Release-Tag muss auf den dokumentierten Quellcommit zeigen; `main` und Tag dürfen nicht versehentlich auseinanderlaufen.
4. Genau ein stabiles Produkt-Release und genau ein Asset `firmware-esp32dev.bin` veröffentlichen. Frühere Releases sind historische Versionen, keine zweite aktuelle Firmwarelinie.
5. Öffentlich verfügbare Dateien, Binärdatei und erreichbare Historie auf Kennwörter, WLAN-Daten, private Anlagenkonfigurationen, private IPs und nicht freigegebene personenbezogene Daten prüfen. Projektkontakt und ausdrücklich freigegebene Urheberangaben sind zulässig.
6. Nach Upload öffentliche Release-API, neueste stabile Version, Tag, genau ein Asset, Bytezahl und SHA-256 prüfen. Den anonymen Direktdownload `.../releases/latest/download/firmware-esp32dev.bin` vollständig laden und dessen SHA-256 mit dem lokalen Paket vergleichen.
7. Erst nach erfolgreicher Prüfung Veröffentlichung als `PASS` dokumentieren. Ein Upload oder sichtbarer GitHub-Eintrag allein genügt nicht.

### Kurze Versionshinweise

- Versionshinweise für Anwender bleiben kurz und nennen nur Änderungen, die bei der Entscheidung für ein Update wirklich wichtig sind.
- Wenn keine neue, relevante Funktion hinzugekommen ist, genügt eine knappe Formulierung wie **„Fehlerbehebungen und Stabilitätsverbesserungen.“** Keine künstlich aufgeblähten Listen und keine Routinearbeiten als neue Produktfunktion darstellen.
- Eine wichtige Fehlerkorrektur darf in einem kurzen Satz genannt werden. Dabei ausdrücklich sagen, dass die bestehende Funktion korrigiert wurde, wenn sie schon vorher vorhanden war. Nicht den Eindruck erwecken, die Funktion sei neu.
- Technische Ursachen, Randfälle, Testmethoden, Prüfsummen und offene Punkte gehören ins Prüfprotokoll, nicht in den kurzen Updatehinweis.
- Releasehinweise nach der Veröffentlichung nicht veralten lassen: bei später bestätigten Änderungen am Dienststatus die Beschreibung korrigieren, ohne dafür eine neue Firmwareversion vorzutäuschen.

## Prüfstatus – Bedeutung und Grenzen

- **STATIC PASS:** definierter Quelltext-/Dokumentenbereich wurde gelesen und geprüft.
- **BUILD PASS:** genau die dokumentierte Buildumgebung wurde erfolgreich gebaut.
- **PACKAGE PASS:** vollständiges OTA-Paket wurde erzeugt und strukturell sowie per Hash geprüft.
- **SERVICE PASS:** der konkret benannte Live-Dienst wurde mit den dokumentierten Anfragen geprüft.
- **REAL-HARDWARE PASS:** Szenario wurde am benannten realen Gerät ausgeführt und Zustand anschließend unabhängig zurückgelesen.
- **FAIL:** Prüfung lief und scheiterte; Fehler und Folgen stehen im Protokoll.
- **OFFEN:** nicht ausgeführt, nicht eindeutig oder nicht ausreichend belegt.

Ein bestandener niedrigerer Prüfstatus ersetzt keinen höheren: Quelltextprüfung ist kein Build, Build ist kein Paket-, Dienst- oder Hardwaretest. Screenshots und Bedieneraussagen können Hinweise liefern, aber einen unabhängigen Readback nicht ersetzen.

## Harte Veröffentlichungsstopps

Nicht veröffentlichen bei: ungeklärten Dokument-/Quellwidersprüchen, falscher oder uneinheitlicher Version, fehlerhaftem Manifest, fehlendem OTA-Bestandteil, Hash-/Größenabweichung, privaten Zugangsdaten, unbeabsichtigten Konfigurationsdaten, nicht erklärten Änderungen an gespeicherten Daten, fehlender Sicherungs-/Rückfallbeschreibung oder einem nicht funktionierenden versprochenen Download. Fehlende reale Geräteprüfung muss deutlich genannt werden und darf nur dann offenbleiben, wenn die konkrete Veröffentlichung dadurch nicht als ungeprüfte Gerätefunktion ausgegeben wird.

## Datensparsamkeit

Öffentliche Unterlagen und GitHub dürfen keine WLAN-Zugangsdaten, SMTP-Geheimnisse, privaten Konfigurationsdateien, privaten Gerätekennungen oder unnötigen Standortdaten enthalten. Der ausdrücklich freigegebene Projektkontakt ist zulässig. Private Sicherungen und Zugangsdaten bleiben außerhalb des Repositories.

Eine neue Installation enthält keine persönlichen Geräte- oder WLAN-Daten. Normales Firmware-OTA bewahrt vorhandene Geräteeinstellungen. Anwender müssen verständlich gewarnt werden, bevor eine Aktion Daten löscht.

## Öffentliche Repository-Historie

Das Entfernen einer Datei aus dem neuesten Stand löscht sie nicht aus älteren Git-Commits. Vor dem Öffentlichschalten erreichbare Branches, Tags und Releases prüfen. Wenn private Daten in der erreichbaren Historie liegen, Veröffentlichungsstand bereinigen oder die alte Ablage privat lassen; nur den aktuellen Dateiinhalt zu säubern reicht nicht.
