# Mail-Endpunkt für Fehlerberichte

Dieses Verzeichnis enthält den PHP-Endpunkt für den späteren Versand von Fehler- und Wunschmeldungen. Das Hosting muss PHP 7.4 oder neuer mit OpenSSL sowie ausgehende SMTP-Verbindungen über SSL/TLS auf Port 465 erlauben. Diese Voraussetzungen wurden für das konkrete do1anb.de-Webhosting noch nicht live bestätigt.

## Dateien und Schutz

- `index.php` kommt in einen eigenen HTTPS-geschützten Unterordner, zum Beispiel `/antennencontroller/melden/`.
- `mail-config.php.example` ist nur eine Vorlage. Eine befüllte Konfiguration mit Zugangsdaten gehört **außerhalb des öffentlich erreichbaren Webverzeichnisses**. Den vollständigen Pfad dazu setzt der Hoster als Umgebungsvariable `ANTCTRL_REPORT_CONFIG`.
- Falls Alfahosting keine solche Variable anbietet, bitte vor dem Einsatz mit dem Hosting-Support klären, wie PHP-Geheimnisse außerhalb des Webverzeichnisses abgelegt werden. Zugangsdaten nicht einfach in einen öffentlichen Ordner hochladen.
- SMTP-Server, Benutzername und Kennwort stehen in der privaten Zugangsdaten-Datei des Betreibers. Sie gehören ausschließlich in die geschützte Serverkonfiguration. Port `465` mit SSL/TLS. Berichte gehen fest an `andreas.bodyn@gmx.de`.

## Ablauf

Der Endpunkt akzeptiert einen Bericht per HTTPS-POST, zeigt zuerst eine Vorschau und sendet erst nach einem zweiten bewussten Klick. Er speichert den Bericht nicht als Datei. Pflichtfelder und Größe werden geprüft; der Versand wird auf fünf Nachrichten pro Stunde je Absender-IP begrenzt. Die Firmware muss den Nutzertext als Formularfelder senden und darf keine WLAN-, Anlagen-, GPIO- oder Backupdaten automatisch beilegen.

Die Firmware-Einbindung und der echte Ende-zu-Ende-Versandtest sind noch erforderlich, bevor der Endpunkt als fertige Funktion veröffentlicht werden kann. Wenn PHP/SMTP auf dem Hosting nicht verfügbar ist, nicht auf unverschlüsseltes HTTP ausweichen.
