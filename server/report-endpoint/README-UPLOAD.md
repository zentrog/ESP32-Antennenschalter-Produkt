# Mail-Endpunkt für Fehlerberichte

Dieses Verzeichnis enthält den öffentlichen PHP-Endpunkt für Fehler- und Wunschmeldungen. Für do1anb.de wurden PHP, HTTPS, SMTP-SSL/TLS auf Port 465 sowie der tatsächliche Berichtversand am 10.10.2026 live geprüft.

## Dateien und Schutz

- `index.php` kommt in einen eigenen HTTPS-geschützten Unterordner, zum Beispiel `/antennencontroller/melden/`.
- `mail-config.php.example` ist nur eine Vorlage. Eine befüllte Konfiguration gehört außerhalb des öffentlichen Webverzeichnisses, wenn der Hoster das unterstützt; dafür wird `ANTCTRL_REPORT_CONFIG` gesetzt.
- SMTP-Zugangsdaten gehören nie zu GitHub oder in ein öffentliches Paket. Das do1anb.de-Paket liegt getrennt im privaten Geheimnis-Ordner.
- Berichte gehen fest an `andreas.bodyn@gmx.de`; der Versand erfordert HTTPS-Vorschau und eine zweite bewusste Bestätigung.

## Live-Prüfung

Am 10.10.2026 lieferte der Health-Endpunkt HTTP 200 mit `{"ready":true}`. Zwei bestätigte Berichte wurden vom Mailserver angenommen und kamen im GMX-Postfach an. GMX sortierte sie trotz SPF-, DKIM- und DMARC-PASS in Spam ein; der Betreiber akzeptiert dies für den alleinigen Empfänger. Die konkrete Diagnoseansicht auf einem ESP und Fehler-/Fallbackfälle sind noch gesondert zu prüfen.

Wenn PHP/SMTP nicht verfügbar ist, nicht auf unverschlüsseltes HTTP ausweichen.
