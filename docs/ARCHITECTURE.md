# Architektur – ESP32-Antennenschalter

Arbeitsstand: Firmware 1.8.36 / API 8, auf dem Test-ESP geprüft; Veröffentlichung noch offen. Die Diagnoseansicht enthält das freiwillige Fehler-/Wunschformular mit HTTPS-Vorschau und lokalem Download-Fallback. Unabhängige Signalwege können gleichzeitig aktiv sein; der Master speichert die aktiven Signalweg-IDs dauerhaft, damit kombinierte Relaiszustände nicht als zusätzliche Signalwege fehlinterpretiert werden.

Neue Installationen und Factory-Resets beginnen ohne WLAN-Zugangsdaten, persönliche Identität, Relais, Funktionen oder logische Geräte. Sie enthalten die sieben öffentlichen NewsTicker-Standardquellen; nur n-tv ist standardmäßig aktiviert. Die WLAN-Zugänge werden durch den Benutzer eingerichtet. Ein normales OTA-Update erhält die gespeicherten Geräteeinstellungen. Schlägt der LittleFS-Mount fehl, formatiert der Startvorgang das Konfigurations-Dateisystem nicht automatisch.

Release-OTA-Dateien enthalten neben dem normalen ESP32-Programmabbild ein CRC32-geprüftes Paket mit Brotli- und gzip-Varianten von `index.html`, `app.js`, `responsive.css` und `style.css` im unbenutzten Ende derselben OTA-Programmpartition. Ein neuer Footer-Marker unterscheidet dieses Format von älteren Paketen. Der OTA-Handler schreibt die vollständige Datei in die inaktive Programmpartition und formatiert LittleFS nicht. Firmware 1.8.10 bis 1.8.15 liefern bei einem gültigen UI-Paket alle vier Hauptdateien ausschließlich aus diesem Paket aus: gzip wird bevorzugt, Brotli nur bei passender Anfrage. Bei nicht unterstützter Komprimierung antwortet sie mit HTTP 406, statt ältere Dateien aus LittleFS zu zeigen. Nach bestätigter Paketprüfung entfernt der normale Start die veralteten LittleFS-Kopien genau dieser vier Dateien und weist neue Einzeldatei-UI-Uploads ab. `/setup.html`, Logo, Favicon, WLAN und Gerätekonfiguration bleiben unberührt; Logo und Favicon sind im Paketbau per Prüfsumme gegen unbeabsichtigte Abweichung geschützt. Fehlt ein gültiges Paket, bleiben die LittleFS-Dateien als Installations-/Rückfallweg erhalten. Damit aktualisiert das vollständige OTA-Asset Firmware und Hauptoberfläche gemeinsam, während WLAN- und Gerätekonfiguration erhalten bleiben.

Die Update-Seite zeigt die neueste stabile GitHub-Version an und führt durch den manuellen Download- und Uploadablauf: Sicherung dieses ESP herunterladen, das einzelne Release-Asset `firmware-esp32dev.bin` laden, die Datei auswählen und hochladen. Der Webserver schreibt den Upload blockweise in die inaktive OTA-Partition und aktiviert ihn nur nach erfolgreicher Firmware-Abbildprüfung. Vorher wird zusätzlich eine interne Sicherung erstellt. LittleFS wird nicht formatiert. Im Verbund muss jeder ESP separat über seine eigene Adresse gesichert und aktualisiert werden. Controller-ID und IP stehen im Sicherungsdateinamen.

Die Wetteranzeige lässt sich unter **Konfigurieren → Dienste** unabhängig von der Blitzwarnung abschalten. Ohne Wetteranzeige wird keine Vorhersage abgerufen. Eine aktive Blitzwarnung kann weiterhin die gemeinsame Position aus der PLZ verwenden. Die PLZ-Koordinaten werden separat zwischengespeichert; Wetter wird nach einem erfolgreichen Abruf alle 30 Minuten aktualisiert, nach einem Fehler nach 5 Minuten erneut versucht und zeigt bis dahin den letzten gültigen Stand.

Unvollständige Konfigurationsanfragen werden abgewiesen. Vorhandene Signalwege können nur durch eine ausdrücklich bestätigte Löschaktion reduziert werden. Die gemeinsame Konfiguration wird vor dem Austausch zusätzlich als vorherige Generation auf dem ESP gehalten.

## Implementierter Bestand
Einzelcontroller führen Schaltaktionen über den lokalen RelayEngine aus. WebUI, externe API und Verbundaufrufe sollen diesen Steuerkern verwenden. GPIO-, Board- und Relaiszuordnung bleiben lokal. Gemeinsame Systemdaten werden über Revisionen zwischen zugeordneten Controllern synchronisiert.

Die nicht benötigten Alias-Routen `/ext/status` und `/ext/execute` sowie der Schalter `features.externalApi` sind in Firmware 1.8.35 entfernt. Reguläre `/api/...`-Schnittstellen für Bedienoberfläche, Verbund, Update und spätere Windows-App bleiben bestehen. Ältere Konfigurationen mit dem zusätzlichen `features.externalApi`-Feld werden weiter eingelesen; das unbekannte Alt-Feld wird ignoriert und beim Speichern nicht mehr ausgegeben.

**Ortswetter anzeigen** und **Blitzwarnung aktiv** sind getrennte Einstellungen unter **Konfigurieren → Dienste**. Die PLZ unter **Geräteidentität** ist der gemeinsame Standort. Bei abgeschalteter Wetteranzeige stoppt der Wettervorhersageabruf; eine aktive Blitzwarnung kann die PLZ-Koordinaten unabhängig davon auflösen. Konfigurationen ohne `weather.enabled` behalten den bisherigen Standard bei und aktivieren das Ortswetter.

Die Verbunderkennung nutzt UDP und den Marker ANTCTRL3. Controller können als Master und Follower zugeordnet werden. Der konfigurierte Master bleibt dauerhaft; ein geeigneter Follower kann vorübergehend koordinieren. Der Code enthält Zustände für Rückkehr und Wiederübernahme durch den permanenten Master.

Ab Firmware 1.8.36 können mehrere unabhängige Funkwege gleichzeitig aktiv sein. Ein Funkgerät bleibt auf höchstens einem aktiven Weg. Gemeinsame Schaltgruppen mit unterschiedlichen Relaisfunktionen sowie ausdrücklich exklusive Anlagenteile verhindern weiterhin den Parallelbetrieb. Beim Wechsel werden nur tatsächlich kollidierende Wege abgeschaltet; unabhängige Wege bleiben eingeschaltet. Bei einem fehlgeschlagenen Schaltversuch werden nur die betroffenen Relaisgruppen auf AUS gesetzt.

## Peer-Status in der Bedienoberfläche
Der Master kann einen Peer beim kurzen Snapshot-Timeout weiterhin als online führen, aber ohne dessen Geräte- und Funktionsdaten liefern. Der Browser verwendet in diesem Fall den letzten bekannten Snapshot nur zur Anzeige und sperrt Schaltaktionen für diesen Peer, bis ein vollständiger Snapshot bestätigt wurde. Damit bleibt das Bedienlayout stabil, ohne einen veralteten Gerätezustand als schaltbar auszugeben.

## Grenzen
Die Verbundkoordination verwendet Heartbeats und Prioritäten. Das beweist keine harte Split-Brain-Sicherheit. Lokale Schutzregeln haben Vorrang.

Die gemeinsame Konfiguration kann Radio-/Antennen-Endpunkte, vollständige Signalwege und Schaltgruppen speichern. Die Bedienung wählt zuerst ein Funkgerät und danach eine freigegebene Antenne; dieser Klick schaltet den vollständigen Weg über alle beteiligten Controller. Ein Wechsel trennt zuvor aktive, widersprechende Wege derselben Schaltgruppen. Bänder und Ressourcen werden geprüft; ein verteilter Simulator, ein umfassender Ressourcenplanner, RX/TX/UNKNOWN und OutputProvider bleiben Zielarbeit.

Node-lokale GPIO-, Board- und Relaiszuordnung darf durch globale Synchronisierung oder fremde Backups nicht überschrieben werden.

Jeder Controller speichert seine aktiven statischen Schaltgruppen und unabhängigen Strom-Ein/Aus-Taster lokal in NVS. Nach einem Neustart initialisiert er zunächst alle konfigurierten Ausgänge in AUS-Stellung und stellt danach seine gespeicherten statischen Auswahlen wieder her. Schlägt das wegen einer aktiven TX-Sperre fehl, bleibt der gespeicherte Zustand erhalten und die Wiederherstellung wird nach Freigabe erneut versucht; auch andere fehlgeschlagene Wiederherstellungen löschen den gespeicherten Zustand nicht. Das gilt für zugeordnete Master- und Follower-Geräte; jeder Knoten schaltet nur seine eigenen Relais. Eine unterbrochene H/V-Zeitaktion wird nicht fortgesetzt, weil die Rotorposition dabei unbekannt werden kann. Sie wird als `UNKNOWN` gemeldet.

Build, Simulation und reale Mehrgerätebeobachtung werden getrennt in docs/VALIDATION.md dokumentiert.

Im klassischen ESP32-Boardprofil dürfen GPIO2/5/12/15 (und GPIO0 im 38-Pin-Profil) als Relaisausgänge konfiguriert werden. Die Oberfläche kennzeichnet sie als Strapping-Risikopins und verlangt eine bewusste Bestätigung. UART0-, Flash- und reine Eingangspins bleiben ausgeschlossen. Ein Warnhinweis ersetzt keine Prüfung der realen Relaisbeschaltung; GPIO12 kann beim Reset die Flash-Versorgungsauswahl beeinflussen.

