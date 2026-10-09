# HTTP/JSON-API – Firmware 1.8.5

Die registrierten Routen stehen in src/WebUi.cpp. Vorhandene Handler beweisen nicht die Laufzeitfunktion.

GET `/`, `/app.js` und `/responsive.css` liefern bei einem gültigen OTA-Oberflächenpaket die Brotli-komprimierten Dateien aus der laufenden Programmpartition. Ohne gültiges Paket greift die Firmware auf LittleFS zurück. `/style.css` wird weiterhin aus LittleFS geliefert. Firmware 1.8.5 ändert die hier aufgeführten API-Routen nicht.

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
GET/PUT /api/controller/config
POST /api/controller/test-relay, /api/controller/restart, /api/controller/factory-reset

## WLAN / Update / externe API
GET /api/update/check prüft das neueste stabile GitHub-Release. Der Aufruf zeigt nur die Version an und startet kein Update.
/api/wifi, /api/wifi/saved, /api/wifi/restore, /api/setup-mode
/api/update/check, /api/update-backup, /api/ui-upload, /update, /rollback
GET /ext/status, POST /ext/execute

Administrative Handler verwenden die konfigurierte Admin-Authentisierung, soweit im jeweiligen Route-Handler registriert. Interne Verbundauthentisierung und lokale Safety sind getrennte Themen. Der Verbundmarker ist keine kryptografische Node-Authentisierung.

`GET /api/combined` liefert einen ETag. Mit `If-None-Match` antwortet der ESP bei unverändertem Anlagen-/Konfigurations-/Peerstatus mit `304 Not Modified`; Uhrzeit und Motor-Restzeiten werden dabei ausgelassen. `GET /api/time` liefert die kleinen, laufend wechselnden Uhrzeitfelder separat. Der Browser zeichnet die Steueransicht nur bei Änderungen neu.

Fehlen bei einem Peer-Snapshot dessen Funktionen oder Geräte, markiert die Browseroberfläche den übernommenen letzten Snapshot als veraltet, zeigt die Kacheln weiter an und sperrt ihre Aktionen bis zu einem vollständigen neuen Snapshot.

Die Bedienoberfläche verwendet die in `/api/shared` gespeicherten Signalwege als Freigabeliste: ein Funkgerät wird gewählt, danach eine zulässige Endantenne. Der zugehörige Eintrag wird über `/api/route/activate` vollständig über beteiligte Steuergeräte geschaltet; `/api/route/deactivate` trennt ihn.

Statische aktive Funktionsgruppen werden auf jedem ESP lokal gespeichert und beim Start auch auf zugeordneten Master-/Follower-Geräten wieder eingeschaltet. `/api/snapshot` und `/ext/status` liefern den bestätigten Laufzeitzustand. Eine bei Reset/Stromausfall unterbrochene H/V-Zeitaktion wird nicht wieder gestartet; ihre Rotorposition gilt als unbekannt.


`GET /api/board` kennzeichnet Strapping-Pins als `caution`. Die Relaiskonfiguration akzeptiert sie nach ausdrücklicher UI-Bestätigung; GPIO12/15/5/2 und im 38-Pin-Profil GPIO0 bleiben als bewusst riskante Ausgänge sichtbar. UART0-, Flash- und reine Eingangspins werden weiterhin durch die Backendvalidierung abgelehnt.



