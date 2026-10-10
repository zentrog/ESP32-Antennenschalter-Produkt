# Fehler- und Wunschmeldungen

Die Firmware 1.8.8 enthält unter **Diagnose** ein freiwilliges Formular für Fehler, Verbesserungswünsche und sonstige Hinweise. Betreff und Beschreibung sind Pflichtfelder; Schritte, Erwartung, Zeitpunkt/Änderung, Name und Rückkontakt sind freiwillig. Die Firmware zeigt die mitgesendete Versionsnummer ausdrücklich an. Sie hängt weder WLAN-Daten noch Rufzeichen, Postleitzahl, GPIO-/Anlagenkonfiguration, IP-/MAC-Adresse oder Backups an. Freitext kann dennoch persönliche Angaben enthalten; der Nutzer muss ihn vor dem Versand prüfen.

## Versandablauf

1. Der Nutzer füllt das Formular aus und bestätigt ausdrücklich, dass er den Text geprüft hat.
2. Die Firmware öffnet eine HTTPS-Vorschauseite auf `https://do1anb.de/antennencontroller/melden/`.
3. Erst nach einer weiteren bewussten Bestätigung auf dieser Vorschauseite versendet der Server eine E-Mail an `andreas.bodyn@gmx.de`.
4. Wenn der Onlineversand nicht klappt, kann der Nutzer stattdessen nur lokal eine JSON-Datei herunterladen. Dafür ist kein E-Mail-Programm nötig.

Es werden keine Berichte dauerhaft als Datei gespeichert. Für die Ratenbegrenzung legt der Endpunkt einen Hash der Absender-IP mit Sendezeitpunkten in einem temporären Serververzeichnis ab und entfernt abgelaufene Einträge bei späteren Aufrufen. Die tatsächliche Aufbewahrung hängt zusätzlich von der Bereinigung des temporären Verzeichnisses beim Hoster ab. Webhoster und SMTP-Anbieter können eigene technische Verbindungsprotokolle führen. Das muss vor dem öffentlichen Einschalten des Endpunkts in einem Datenschutzhinweis auf der Website erklärt werden.

Die Diagnoseansicht prüft vorab, ob der Endpunkt über HTTPS erreichbar ist und ob die konfigurierte Mailbox die SMTP-Anmeldung annimmt. Die Prüfung verschickt keine E-Mail und ist auf 30 Aufrufe pro Stunde und Absender-IP begrenzt. Bei einem Fehler bleibt die Schaltfläche zum lokalen Berichtdownload verfügbar und der Mailversand wird deaktiviert.

## Installation des Mail-Endpunkts

Das öffentliche Vorlagenpaket `do1anb-mail-endpoint-upload.zip` enthält ausschließlich `index.php`; es hat keine SMTP-Zugangsdaten und ist nicht das personalisierte Paket für do1anb.de. Für den konkreten Webspace wurde ein privates ZIP erstellt. Es enthält `melden/index.php`, `melden/mail-config.php` mit den Zugangsdaten des eigens angelegten Absendekontos sowie `melden/.htaccess`. Dieses private Paket liegt außerhalb des Repositories im privaten Geheimnis-Ordner und darf nicht zu GitHub hochgeladen oder öffentlich geteilt werden. Eine lokale Anleitung liegt daneben. Das ZIP enthält nur `melden/`, damit es in einen bereits vorhandenen Ordner `/antennencontroller/` entpackt werden kann, ohne einen doppelten Ordner anzulegen.

Benötigt werden PHP 7.4+ mit OpenSSL und ausgehendes SMTP-SSL/TLS auf Port 465. Das personalisierte ZIP wird im vorhandenen Webordner `/antennencontroller/` entpackt; sein enthaltenes Verzeichnis `melden/` erzeugt den benötigten Pfad. `.htaccess` blockiert den direkten Zugriff auf der Apache-Ebene; der private PHP-Konfigurationsbaustein liefert außerdem bei einem direkten Aufruf einen 404-Fehler. Auf Nginx wird `.htaccess` ignoriert. Vor der Nutzung deshalb prüfen, dass PHP-Dateien tatsächlich ausgeführt und nicht als Quelltext ausgeliefert werden und ein direkter Aufruf von `/antennencontroller/melden/mail-config.php` einen leeren 404 liefert. Wenn PHP nicht aktiv ist oder die Zugangsdaten sichtbar werden, das Paket sofort vom Webspace entfernen und die Konfiguration außerhalb des Webverzeichnisses ablegen; PHP erhält den Pfad über `ANTCTRL_REPORT_CONFIG`. Unverschlüsseltes HTTP ist gesperrt.

Der öffentliche Health-Aufruf `https://do1anb.de/antennencontroller/melden/?health=1` lieferte am 10.10.2026 HTTP 404 mit `Server: nginx`. Der Upload ist deshalb noch erforderlich. Da Nginx `.htaccess` nicht verarbeitet, nach dem Entpacken zuerst den Health-Aufruf und danach den direkten Aufruf von `mail-config.php` prüfen. Erst wenn der Health-Aufruf `{"ready":true}` liefert, `mail-config.php` direkt einen leeren 404 liefert und ein bewusst bestätigter Testbericht angekommen ist, gilt der Mailversand als eingerichtet.

## Freigabe-Gate

Die Implementierung im Browser und das Upload-Paket allein beweisen noch keine Zustellung. Vor einem öffentlichen Firmware-Release mit aktivem Versand sind auf dem echten do1anb.de-Hosting PHP-Version, HTTPS, SMTP-Login, korrekte Empfängeradresse, Vorschau/Bestätigungsablauf, ungültige Eingaben, Größenlimit, Rate-Limit, Fehleranzeige und der JSON-Download-Fallback zu prüfen. Bis dahin ist die Versandfunktion **nicht produktiv freigegeben**; veröffentlichte Firmware darf keinen dauerhaft defekten Versandknopf enthalten.
