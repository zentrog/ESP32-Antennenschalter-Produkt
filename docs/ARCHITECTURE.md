# Architektur – ESP32-Antennenschalter

Quellstand: Firmware 1.8.6 / API 6.

Neue Installationen und Factory-Resets beginnen ohne WLAN-Zugangsdaten, persönliche Identität, Relais, Funktionen oder logische Geräte. Die WLAN-Zugänge werden durch den Benutzer eingerichtet. Ein normales OTA-Update erhält die gespeicherten Geräteeinstellungen. Schlägt der LittleFS-Mount fehl, formatiert der Startvorgang das Konfigurations-Dateisystem nicht automatisch.

Release-OTA-Dateien enthalten neben dem normalen ESP32-Programmabbild ein komprimiertes, CRC32-geprüftes Paket aus `index.html`, `app.js` und `responsive.css` im unbenutzten Ende derselben OTA-Programmpartition. Der OTA-Handler schreibt die vollständige Datei in die inaktive Programmpartition und formatiert LittleFS nicht. Die Firmware erkennt das Paket anhand seines Endmarkers und liefert die Webdateien daraus aus; fehlt ein gültiges Paket, verwendet sie LittleFS. Damit erfolgen Programm- und Weboberflächenupdate über eine Datei und die vorhandenen Konfigurationsdateien bleiben erhalten.

Die Online-Updateprüfung fragt die neueste stabile Veröffentlichung bei GitHub ab und zeigt sie an. Die Firmware wird niemals automatisch installiert. Vor einem Firmware-Update muss der Benutzer auf der Update-Seite eine geprüfte Sicherungsdatei mit lokaler Konfiguration dieses ESP, gemeinsamer Konfiguration und dessen WLAN-Daten herunterladen und bestätigen. Im Verbund werden Slave-Konfigurationen nicht vom Master exportiert; für eine vollständige Verbundsicherung wird jeder ESP einzeln über seine eigene Adresse gesichert. Dateiname und Datei enthalten Controller-ID und IP.

Der Wetterdienst aktualisiert nach einem erfolgreichen Abruf alle 30 Minuten. Schlägt ein Abruf fehl, bleibt der letzte gültige Stand sichtbar und der nächste Versuch erfolgt nach 5 Minuten.

Unvollständige Konfigurationsanfragen werden abgewiesen. Vorhandene Signalwege können nur durch eine ausdrücklich bestätigte Löschaktion reduziert werden. Die gemeinsame Konfiguration wird vor dem Austausch zusätzlich als vorherige Generation auf dem ESP gehalten.

## Implementierter Bestand
Einzelcontroller führen Schaltaktionen über den lokalen RelayEngine aus. WebUI, MQTT, externe API und Verbundaufrufe sollen diesen Steuerkern verwenden. GPIO-, Board- und Relaiszuordnung bleiben lokal. Gemeinsame Systemdaten werden über Revisionen zwischen zugeordneten Controllern synchronisiert.

Die Verbunderkennung nutzt UDP und den Marker ANTCTRL3. Controller können als Master und Follower zugeordnet werden. Der konfigurierte Master bleibt dauerhaft; ein geeigneter Follower kann vorübergehend koordinieren. Der Code enthält Zustände für Rückkehr und Wiederübernahme durch den permanenten Master.

## Peer-Status in der Bedienoberfläche
Der Master kann einen Peer beim kurzen Snapshot-Timeout weiterhin als online führen, aber ohne dessen Geräte- und Funktionsdaten liefern. Der Browser verwendet in diesem Fall den letzten bekannten Snapshot nur zur Anzeige und sperrt Schaltaktionen für diesen Peer, bis ein vollständiger Snapshot bestätigt wurde. Damit bleibt das Bedienlayout stabil, ohne einen veralteten Gerätezustand als schaltbar auszugeben.

## Grenzen
Die Verbundkoordination verwendet Heartbeats und Prioritäten. Das beweist keine harte Split-Brain-Sicherheit. Lokale Schutzregeln haben Vorrang.

Die gemeinsame Konfiguration kann Radio-/Antennen-Endpunkte, vollständige Signalwege und Schaltgruppen speichern. Die Bedienung wählt zuerst ein Funkgerät und danach eine freigegebene Antenne; dieser Klick schaltet den vollständigen Weg über alle beteiligten Controller. Ein Wechsel trennt zuvor aktive, widersprechende Wege derselben Schaltgruppen. Bänder und Ressourcen werden geprüft; ein verteilter Simulator, ein umfassender Ressourcenplanner, RX/TX/UNKNOWN und OutputProvider bleiben Zielarbeit.

Node-lokale GPIO-, Board- und Relaiszuordnung darf durch globale Synchronisierung oder fremde Backups nicht überschrieben werden.

Jeder Controller speichert seine aktiven statischen Schaltgruppen lokal in NVS. Nach einem Neustart initialisiert er zunächst alle konfigurierten Ausgänge in AUS-Stellung und stellt danach seine gespeicherten statischen Auswahlen wieder her. Das gilt für zugeordnete Master- und Follower-Geräte; jeder Knoten schaltet nur seine eigenen Relais. Eine unterbrochene H/V-Zeitaktion wird nicht fortgesetzt, weil die Rotorposition dabei unbekannt werden kann. Sie wird als `UNKNOWN` gemeldet.

Build, Simulation und reale Mehrgerätebeobachtung werden getrennt in docs/VALIDATION.md dokumentiert.

Im klassischen ESP32-Boardprofil dürfen GPIO2/5/12/15 (und GPIO0 im 38-Pin-Profil) als Relaisausgänge konfiguriert werden. Die Oberfläche kennzeichnet sie als Strapping-Risikopins und verlangt eine bewusste Bestätigung. UART0-, Flash- und reine Eingangspins bleiben ausgeschlossen. Ein Warnhinweis ersetzt keine Prüfung der realen Relaisbeschaltung; GPIO12 kann beim Reset die Flash-Versorgungsauswahl beeinflussen.

