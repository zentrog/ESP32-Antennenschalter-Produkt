# HTTP/JSON-API – Firmware 1.8.23 / API 7

Die registrierten Routen stehen in src/WebUi.cpp. Vorhandene Handler beweisen nicht die Laufzeitfunktion.

GET `/`, `/app.js`, `/responsive.css` und `/style.css` liefern bei einem gültigen OTA-Oberflächenpaket die Brotli- oder gzip-Dateien aus der laufenden Programmpartition. Fehlt das geprüfte Paket, bleibt LittleFS als Rückfall für Erstinstallationen verfügbar. Bei einem gültigen Paket entfernt der normale Start die veralteten LittleFS-Kopien dieser vier Dateien; Konfiguration, WLAN, Sicherungen und `/setup.html` bleiben unberührt. Der Einzeldatei-Upload `/api/ui-upload` wird bei gültigem Paket abgewiesen, damit kein veralteter Oberflächenstand erneut abgelegt wird. Logo und Favicon bleiben LittleFS-Dateien und werden vom Paketbauer auf unveränderte Prüfsummen geprüft. Die optionale Fehler-/Wunschmeldung nutzt den dokumentierten externen HTTPS-Endpunkt.

## Steuerung und Status
GET /api/snapshot, /api/combined, /api/time, /api/peers
POST /api/execute, /api/route/activate, /api/route/deactivate
POST /api/stop, /api/system/storm, /api/federation/storm
GET/PUT /api/config, /api/shared; PUT /api/layout/item
GET /api/board, /api/errors; DELETE /api/errors
GET /api/news; POST /api/news/refresh; GET /api/weather. Bei News kann `sources[].stale` zwischengespeicherte Meldungen kennzeichnen; Wetter liefert `stale=true`, `valid=true` und `error` zusammen, wenn der letzte erfolgreiche Stand wegen eines Abruffehlers weiter angezeigt wird.

## Cluster / Verbund
GET/PUT /api/federation/config
POST /api/system/admission, /api/system/adopt, /api/system/make-master
POST /api/federation/adopt, /api/federation/make-master, /api/federation/stop
POST /api/federation/group/off, /api/federation/test-relay, /api/federation/restart, /api/federation/factory-reset, /api/federation/pull-shared
POST /api/provisioning/authorize
GET/PUT /api/controller/config (PUT übernimmt im vollständigen Konfigurationsobjekt auch alle Federation-Metadaten)
POST /api/controller/test-relay, /api/controller/restart, /api/controller/factory-reset

## WLAN / Update / externe API
GET /api/update/check prüft das neueste stabile GitHub-Release. Der Aufruf zeigt nur die Version an und startet kein Update. POST /api/update/install-latest verlangt eine zuvor heruntergeladene Sicherung, erstellt eine interne Sicherung und startet den Download von ausschließlich `firmware-esp32dev.bin` im Hintergrund. GET /api/update/status meldet `running`, `success` oder `error`. Die Browseroberfläche wartet auf die neue Firmwareversion und denselben Controller, bevor sie Erfolg zeigt. LittleFS bleibt erhalten. Der Endpunkt aktualisiert ausschließlich den aufrufenden ESP; Slaves werden nicht automatisch aktualisiert. Das eingebettete Root-Zertifikatsbündel wird mit `tools/convert_github_trust_bundle.py` aus `certs/github-roots.json` im ESP-IDF-Format erzeugt; der Paketbau führt diesen Schritt vor PlatformIO automatisch aus.
/api/wifi, /api/wifi/saved, /api/wifi/restore, /api/setup-mode
/api/update/check, /api/update-backup, /api/update/install-latest, /api/ui-upload, /update, /rollback
GET /ext/status, POST /ext/execute

Administrative Handler verwenden die konfigurierte Admin-Authentisierung, soweit im jeweiligen Route-Handler registriert. Interne Verbundauthentisierung und lokale Safety sind getrennte Themen. Der Verbundmarker ist keine kryptografische Node-Authentisierung.

`GET /api/combined` liefert einen ETag. Mit `If-None-Match` antwortet der ESP bei unverändertem Anlagen-/Konfigurations-/Peerstatus mit `304 Not Modified`; Uhrzeit und Motor-Restzeiten werden dabei ausgelassen. `GET /api/time` liefert die kleinen, laufend wechselnden Uhrzeitfelder separat. Der Browser zeichnet die Steueransicht nur bei Änderungen neu.

Fehlen bei einem Peer-Snapshot dessen Funktionen oder Geräte, markiert die Browseroberfläche den übernommenen letzten Snapshot als veraltet, zeigt die Kacheln weiter an und sperrt ihre Aktionen bis zu einem vollständigen neuen Snapshot.

Die Bedienoberfläche verwendet die in `/api/shared` gespeicherten Signalwege als Freigabeliste: ein Funkgerät wird gewählt, danach eine zulässige Endantenne. Der zugehörige Eintrag wird über `/api/route/activate` vollständig über beteiligte Steuergeräte geschaltet; `/api/route/deactivate` trennt ihn.

Statische aktive Funktionsgruppen und unabhängige Ein/Aus-Schaltfunktionen werden auf jedem ESP lokal gespeichert und beim Start auch auf zugeordneten Master-/Follower-Geräten wieder eingeschaltet. Ein Ein/Aus-Taster verwendet `type=toggle`, eine eigene Gruppe und ein optional noch nicht zugeordnetes Relais. Ohne Relaiszuordnung bleibt der Taster gesperrt. `/api/snapshot` und `/ext/status` liefern den bestätigten Laufzeitzustand. Eine bei Reset/Stromausfall unterbrochene H/V-Zeitaktion wird nicht wieder gestartet; ihre Rotorposition gilt als unbekannt.

Die gemeinsame Oberflächenkonfiguration `ui` enthält `powerInactiveColor` und `powerActiveColor` für die Statusflächen von Anlagenteilen der Kategorie `supply` (Strom). Fehlen die Felder in einer älteren Konfiguration, gelten `#275c91` (inaktiv) und `#259b55` (aktiv). Die allgemeine Farbe `lockedColor` hat Vorrang, wenn ein Stromtaster gesperrt ist.


`GET /api/board` kennzeichnet Strapping-Pins als `caution`. Die Relaiskonfiguration akzeptiert sie nach ausdrücklicher UI-Bestätigung; GPIO12/15/5/2 und im 38-Pin-Profil GPIO0 bleiben als bewusst riskante Ausgänge sichtbar. UART0-, Flash- und reine Eingangspins werden weiterhin durch die Backendvalidierung abgelehnt.



