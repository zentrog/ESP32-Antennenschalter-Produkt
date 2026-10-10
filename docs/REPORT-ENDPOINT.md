# Fehler- und Wunschmeldungen

Die Firmware 1.8.8 enthält unter **Diagnose** ein freiwilliges Formular für Fehler, Verbesserungswünsche und sonstige Hinweise. Betreff und Beschreibung sind Pflichtfelder; Schritte, Erwartung, Zeitpunkt/Änderung, Name und Rückkontakt sind freiwillig. Die Firmware zeigt die mitgesendete Versionsnummer ausdrücklich an. Sie hängt weder WLAN-Daten noch Rufzeichen, Postleitzahl, GPIO-/Anlagenkonfiguration, IP-/MAC-Adresse oder Backups an. Freitext kann dennoch persönliche Angaben enthalten; der Nutzer muss ihn vor dem Versand prüfen.

## Versandablauf

1. Der Nutzer füllt das Formular aus und bestätigt ausdrücklich, dass er den Text geprüft hat.
2. Die Firmware öffnet eine HTTPS-Vorschauseite auf `https://do1anb.de/antennencontroller/melden/`.
3. Erst nach einer weiteren bewussten Bestätigung auf dieser Vorschauseite versendet der Server eine E-Mail an `andreas.bodyn@gmx.de`.
4. Wenn der Onlineversand nicht klappt, kann der Nutzer stattdessen nur lokal eine JSON-Datei herunterladen. Dafür ist kein E-Mail-Programm nötig.

Der Health-Aufruf prüft HTTPS und die SMTP-Anmeldung, verschickt aber keine Nachricht. Die Rate-Limits betragen 30 Health-Abfragen und fünf Berichte pro Stunde je Absender-IP. Berichte werden serverseitig nicht dauerhaft als Datei gespeichert. Temporäre Rate-Limit-Dateien enthalten einen Hash der Absender-IP mit Sendezeitpunkten; Hoster und SMTP-Anbieter können zusätzliche technische Verbindungsprotokolle führen. Datenschutz und Aufbewahrung technischer Protokolle sind vor dem Einsatz in einer Datenschutzerklärung zu beschreiben.

## Installation und Schutz

Das öffentliche Vorlagenpaket `do1anb-mail-endpoint-upload.zip` enthält keine SMTP-Zugangsdaten. Für do1anb.de wurde ein getrenntes privates ZIP mit `melden/index.php`, `melden/mail-config.php` und `melden/.htaccess` erstellt. Es liegt außerhalb des Repositories und darf nicht öffentlich geteilt oder zu GitHub hochgeladen werden. Das dedizierte SMTP-Passwort gehört ausschließlich in dieses private Paket beziehungsweise die geschützte Serverkonfiguration. Das ZIP wird im bestehenden Webordner `/antennencontroller/` entpackt und erzeugt dort nur den Unterordner `melden/`.

Benötigt werden PHP 7.4+ mit OpenSSL, HTTPS und ausgehendes SMTP-SSL/TLS auf Port 465. `.htaccess` schützt Apache; Nginx ignoriert diese Datei. Die PHP-Konfiguration verweigert ebenfalls einen direkten Abruf. Auf dem aktuellen do1anb.de-Hosting liefert der direkte Abruf von `mail-config.php` leer HTTP 403. Zugangsdaten dürfen nie als PHP-Quelltext ausgeliefert werden. Unverschlüsseltes HTTP ist gesperrt.

## Live-Status vom 10.10.2026

- Health-Endpunkt: HTTP 200 mit `{"ready":true}`.
- Direkter Abruf der SMTP-Konfiguration: leer HTTP 403.
- SMTP-Anmeldung und zwei ausdrücklich bestätigte Testberichte: erfolgreich; beide Nachrichten kamen im GMX-Postfach an.
- GMX legte beide Testberichte in Spam ab. Im Header des zweiten Berichts bestanden SPF, DKIM, DMARC und IP-Reverse-Prüfung. Der Betreiber akzeptiert die Spam-Einstufung, weil die Berichte nur an ihn selbst gehen.
- DMARC ist im Überwachungsmodus veröffentlicht: `_dmarc.do1anb.de TXT "v=DMARC1; p=none;"`.
- Das zuerst vorbereitete private ZIP enthielt ein falsches SMTP-Passwort und wurde durch ein korrigiertes Paket ersetzt. Der Fehler ist behoben; Zugangsdaten sind nicht betroffen.
- Die Live-Formularstrecke des Endpunkts ist geprüft. Der vollständige Klickpfad aus der Diagnoseansicht eines physischen ESP, die Fehlerfälle, Größen-/Rate-Limits und der JSON-Download-Fallback brauchen noch einen Gerätetest.

Die SMTP-Bereitschaft ist damit bestätigt. Die Spam-Einstufung durch GMX ist eine empfangerspezifische Bewertung und kein Beleg für einen Versand- oder Authentifizierungsfehler; sie garantiert umgekehrt keine Inbox-Zustellung bei anderen Empfängern.
