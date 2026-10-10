# Fehler- und Wunschmeldungen

Die Firmware 1.8.7 ergänzt unter **Diagnose** ein freiwilliges Formular für Fehler, Verbesserungswünsche und sonstige Hinweise. Betreff und Beschreibung sind Pflichtfelder; Schritte, Erwartung, Zeitpunkt/Änderung, Name und Rückkontakt sind freiwillig. Die Firmware zeigt die mitgesendete Versionsnummer ausdrücklich an. Sie hängt weder WLAN-Daten noch Rufzeichen, Postleitzahl, GPIO-/Anlagenkonfiguration, IP-/MAC-Adresse oder Backups an. Freitext kann dennoch persönliche Angaben enthalten; der Nutzer muss ihn vor dem Versand prüfen.

## Versandablauf

1. Der Nutzer füllt das Formular aus und bestätigt ausdrücklich, dass er den Text geprüft hat.
2. Die Firmware öffnet eine HTTPS-Vorschauseite auf `https://do1anb.de/antennencontroller/melden/`.
3. Erst nach einer weiteren bewussten Bestätigung auf dieser Vorschauseite versendet der Server eine E-Mail an `andreas.bodyn@gmx.de`.
4. Wenn der Onlineversand nicht klappt, kann der Nutzer stattdessen nur lokal eine JSON-Datei herunterladen. Dafür ist kein E-Mail-Programm nötig.

Es werden keine Berichte dauerhaft als Datei gespeichert. Für die Ratenbegrenzung legt der Endpunkt einen Hash der Absender-IP mit Sendezeitpunkten in einem temporären Serververzeichnis ab und entfernt abgelaufene Einträge bei späteren Aufrufen. Die tatsächliche Aufbewahrung hängt zusätzlich von der Bereinigung des temporären Verzeichnisses beim Hoster ab. Webhoster und SMTP-Anbieter können eigene technische Verbindungsprotokolle führen. Das muss vor dem öffentlichen Einschalten des Endpunkts in einem Datenschutzhinweis auf der Website erklärt werden.

Die Diagnoseansicht prüft vorab, ob der Endpunkt über HTTPS erreichbar ist und ob die konfigurierte Mailbox die SMTP-Anmeldung annimmt. Die Prüfung verschickt keine E-Mail und ist auf 30 Aufrufe pro Stunde und Absender-IP begrenzt. Bei einem Fehler bleibt die Schaltfläche zum lokalen Berichtdownload verfügbar und der Mailversand wird deaktiviert.

## Installation des Mail-Endpunkts

`do1anb-mail-endpoint-upload.zip` enthält ausschließlich `index.php`. Das ZIP wird in den HTTPS-Ordner `/antennencontroller/melden/` entpackt. Die Beispielkonfiguration und die ausführliche Anleitung liegen nur im Quellprojekt unter `server/report-endpoint/`; sie sind nicht Teil des Upload-ZIP. Die private SMTP-Konfiguration ist separat und wird weder in Git noch ins Firmware-Image übernommen.

Benötigt werden PHP 7.4+ mit OpenSSL und ausgehendes SMTP-SSL/TLS auf Port 465. SMTP-Host, Benutzername und Kennwort müssen aus der privaten Zugangsdaten-Datei in die Serverkonfiguration übernommen werden; die Werte gehören nicht in dieses öffentliche Repository oder in das Upload-ZIP. Die Mailkonfiguration muss außerhalb des öffentlichen Webverzeichnisses liegen; PHP bekommt ihren Pfad über `ANTCTRL_REPORT_CONFIG`. Wenn der Hoster keine sichere Ablage außerhalb des Webverzeichnisses bereitstellt, darf der Mailendpunkt noch nicht aktiviert werden. Unverschlüsseltes HTTP ist gesperrt.

Zum Erstellungszeitpunkt antwortet die öffentliche Adresse `/antennencontroller/melden/?health=1` mit HTTP 404. Der Upload ist deshalb noch erforderlich; anschließend muss die PHP-Konfiguration eingerichtet und der Ende-zu-Ende-Test bestanden werden.

## Freigabe-Gate

Die Implementierung im Browser und das Upload-Paket allein beweisen noch keine Zustellung. Vor einem öffentlichen Firmware-Release mit aktivem Versand sind auf dem echten do1anb.de-Hosting PHP-Version, HTTPS, SMTP-Login, korrekte Empfängeradresse, Vorschau/Bestätigungsablauf, ungültige Eingaben, Größenlimit, Rate-Limit, Fehleranzeige und der JSON-Download-Fallback zu prüfen. Bis dahin ist die Versandfunktion **nicht produktiv freigegeben**; veröffentlichte Firmware darf keinen dauerhaft defekten Versandknopf enthalten.
