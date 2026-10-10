# Antennensteuerung – Anleitung Schritt für Schritt

Diese Anleitung erklärt den Einstieg ohne Vorwissen. Eine neue Installation startet absichtlich leer: kein WLAN, kein Rufzeichen, keine Postleitzahl, keine Relais und keine Geräte.

## 1. Das brauchst du

- einen ESP32-WROOM-32 oder ein kompatibles ESP32-DevKit
- ein USB-Datenkabel (manche Kabel laden nur und übertragen keine Daten)
- einen Windows-PC mit Internetzugang
- den Namen und das Kennwort deines 2,4-GHz-WLANs
- Relaiskarte und Antennenanlage erst, wenn die Verdrahtung sicher geprüft ist

## 2. Programme auf den PC laden

1. Installiere Visual Studio Code von der offiziellen Microsoft-Seite.
2. Öffne Visual Studio Code und installiere links unter **Erweiterungen** die Erweiterung **PlatformIO IDE**.
3. Lade dieses GitHub-Projekt als ZIP herunter und entpacke es, zum Beispiel nach C:/ESP32-Antennensteuerung.
4. Wähle **Datei → Ordner öffnen** und öffne den entpackten Projektordner. Warte, bis PlatformIO seine Bauteile eingerichtet hat.

## 3. Einen ganz neuen ESP zum ersten Mal programmieren

1. Verbinde den ESP32 per USB-Datenkabel mit dem PC.
2. Wähle in PlatformIO **esp32dev** für das übliche ESP32-DevKit mit 30 Pins. Es gibt nur noch einen Firmware-Build.
3. Nur beim allerersten Einrichten eines neuen Geräts musst du auch das Dateisystem laden: In PlatformIO unter **Project Tasks → esp32dev → Platform → Build Filesystem Image**, danach **Upload Filesystem Image**.
4. Lade anschließend unter **Project Tasks → esp32dev → General → Upload** die Firmware.
5. Wenn der Upload bei „Connecting…“ wartet, halte die Taste **BOOT** am ESP gedrückt. Lass sie los, sobald der Upload beginnt.

> **Achtung:** „Upload Filesystem Image“ überschreibt den Dateispeicher im ESP und kann gespeicherte Einstellungen löschen. Bei einem bereits eingerichteten Gerät niemals das Dateisystem für ein normales Update hochladen. Verwende dafür den normalen Firmware-Updateweg.

## 4. WLAN am Gerät einrichten

1. Nach dem ersten Start sendet der ESP ein eigenes WLAN, zum Beispiel **AntennaController-ABC123**.
2. Verbinde PC oder Handy mit diesem WLAN. Beim ersten Einrichten hat es kein Kennwort.
3. Öffne im Browser **http://192.168.4.1**.
4. Wähle dein 2,4-GHz-WLAN, gib das Kennwort ein und speichere.
5. Der ESP startet neu und verbindet sich mit dem Router. Klappt das nicht, erscheint sein Einrichtungs-WLAN wieder. Prüfe dann Name, Kennwort und 2,4-GHz-Verfügbarkeit.
6. Öffne die IP-Adresse des ESP im Router oder reserviere dort dauerhaft eine IP-Adresse für das Gerät.

WLAN-Zugangsdaten werden im ESP gespeichert, nicht in der öffentlichen GitHub-Version. Ein neuer Slave erhält über die öffentliche Produktversion keine Zugangsdaten. Richte das WLAN am Gerät ein oder übernimm es ausdrücklich über die Verbundfunktion.

## 5. Rufzeichen, Platine und Relais einrichten

1. Öffne **Konfigurieren → Gerät**. Rufzeichen, Name, Standort und Postleitzahl sind freiwillig. Eine leere Postleitzahl schaltet die Wetteranzeige ab.
2. Wähle unter **Platine/GPIO** das Profil deiner tatsächlichen ESP-Platine.
3. Öffne **Relaiskanäle**. Lege für jeden Relais-Eingang einen Namen, GPIO und die passende Logik fest.
4. „Relais EIN bei GPIO LOW“ bedeutet: Der ESP schaltet das Relais ein, indem er den Pin auf LOW (Masse) zieht. Bei HIGH gilt das Gegenteil.
5. Ändere GPIOs nur, wenn du weißt, welcher Draht daran angeschlossen ist. Einige Pins können das Startverhalten des ESP beeinflussen.
6. Teste Relais zuerst ohne angeschlossenen Sender. Sende nicht, solange du die Zuordnung prüfst.

## 6. Funkgeräte und Antennen festlegen

1. Unter **Anlagenteile** legst du deine echten Funkgeräte und Antennen an.
2. Verknüpfe die Geräte mit den passenden Schaltfunktionen.
3. Unter **Signalwege** erlaubst du nur Verbindungen, die es in deiner HF-Verkabelung wirklich gibt.
4. Die Signalwege verhindern ungültige Kombinationen. Sie sind keine Bedienknöpfe.
5. Auf der Startseite klickst du zuerst ein Funkgerät an und danach die gewünschte freigegebene Antenne. Der ESP schaltet den kompletten gespeicherten Weg.

Bei einer drehbaren Yagi erscheinen **Horizontal** und **Vertikal** nur bei der dafür vorgesehenen Antenne. Während der Motor läuft, darf nicht gesendet werden.

## 7. Mehrere ESPs verbinden

1. Verbinde alle ESPs mit demselben lokalen Netzwerk.
2. Richte zuerst den Master ein und schalte ihn ein.
3. Richte die weiteren Geräte als Slaves ein und nimm sie über die Verbundfunktion auf.
4. Prüfe auf der Startseite, dass jeder Slave als erreichbar angezeigt wird.
5. Jeder ESP behält seine eigenen GPIO- und Relais-Einstellungen. Prüfe den Verbund ohne Sender und ohne Motorfahrt.

## 8. Neustart und Updates

Die Bedienseite behält auf Handy, Laptop und Monitor die gespeicherten Positionen der Anlagenteile bei. Bei kleinerem Fenster werden Raster und Schrift proportional verkleinert; die Karten werden nicht automatisch in eine andere Anordnung verschoben.

Ein normaler Neustart stellt gespeicherte statische Schaltzustände wieder her. Eine unterbrochene Motor-Zeitfahrt wird aus Sicherheitsgründen nicht fortgesetzt; ihre Position gilt dann als unbekannt.
Ist die TX-Sperre direkt nach dem Einschalten aktiv, bleiben die Relais zunächst aus. Der gespeicherte Zustand bleibt erhalten und wird wieder eingeschaltet, sobald die TX-Sperre frei ist.

**Neueste Firmware direkt herunterladen** lädt mit einem Klick die einzige vollständige Firmwaredatei von GitHub herunter. Die Release-Seite muss nicht geöffnet und keine Datei daraus ausgewählt werden. Die Onlineprüfung installiert nichts automatisch. Danach startest du **Konfigurieren → Programm/Update → Manuelles OTA** und wählst die heruntergeladene Datei aus. Das Update schreibt nur in den jeweils inaktiven Programmplatz. Nach Prüfung des neuen Oberflächenpakets löscht der ESP nur die vier alten Dateien der Hauptoberfläche aus LittleFS; WLAN, Geräteeinstellungen, Sicherungen und die WLAN-Einrichtungsseite bleiben erhalten.

Vor dem Update muss eine Sicherungsdatei auf deinem Computer gespeichert und bestätigt werden. Auf der manuellen Update-Seite klickst du dazu auf **Konfigurationssicherung herunterladen**. Der ESP öffnet den normalen Browserdownload. Das Kästchen ist anklickbar; setze es erst, nachdem die Datei wirklich auf deinem Computer gespeichert ist. Firmwaredatei und Installationsknöpfe werden dann freigeschaltet. Der ESP prüft zusätzlich selbst, ob die Sicherungsdatei vorher abgerufen wurde, und weist ein Update ohne diesen Abruf zurück. Sie enthält die lokalen Einstellungen des ESP, dessen WLANs und die gemeinsame Anlagenkonfiguration. In einem Verbund reicht die Sicherung des Masters nicht für eine vollständige Wiederherstellung: Öffne jeden ESP über seine eigene Adresse und lade dort separat eine Sicherung herunter. Der Dateiname enthält Controller-ID und IP; dieselben Angaben stehen auch in der Datei. Die interne Sicherung des ESP ersetzt deine Datei auf dem Computer nicht.
### Sicherung nach einer Neuinstallation manuell zurückspielen

1. Verbinde Handy oder PC mit dem Einrichtungs-WLAN `AntennaController-XXXXXX` des ESP. Wenn das Gerät meldet, dass dieses WLAN kein Internet hat, bleib trotzdem verbunden.
2. Öffne `http://192.168.4.1` und richte zunächst dein normales WLAN ein. Das ist nötig, damit du anschließend die vollständige Konfigurationsverwaltung öffnen kannst.
3. Öffne danach die Antennensteuerung über ihre Netzwerkadresse. Gehe zu **Konfigurieren → Sicherheit → Sicherung / Wiederherstellung → Konfiguration importieren** und wähle die zuvor gespeicherte `.json`-Datei aus.
4. Bestätige den Import. Die Sicherungsdatei enthält Gerätekonfiguration einschließlich Verbundzuordnung, gemeinsame Anlagenkonfiguration und WLAN-Zugangsdaten. Verwende für die Wiederherstellung mindestens Firmware 1.8.13, damit auch die interne Verbundkennung übernommen wird. Verwende für jedes Gerät im Verbund die Sicherung, die zu dessen Controller-ID gehört.


Die vollständige OTA-Datei wird mit `node tools/build-ota-package.mjs esp32dev` gebaut. Node.js, npm, PlatformIO und die festgelegte Terser-Version werden benötigt. Der Paketbau prüft die komprimierten Webdateien, ihre Prüfsumme und den verfügbaren Programmspeicherplatz.

Das Wetter wird nach einem erfolgreichen Abruf alle 30 Minuten aktualisiert. Nach einem fehlgeschlagenen Abruf versucht der ESP es nach 5 Minuten erneut und zeigt bis dahin den letzten gültigen Wetterstand an.

Eine Datei für das Dateisystem ist nur für einen neuen ESP oder eine ausdrücklich gewünschte Neuinitialisierung. Sie gehört nicht zu einem normalen Update.

Wenn der ESP sein Dateisystem nicht einhängen kann, formatiert die Firmware es nicht automatisch. Die vorhandene Konfiguration bleibt dadurch vor einem automatischen Löschversuch geschützt. Bei einem Startfehler: kein Reset und kein Dateisystem-Upload auf dem eingerichteten Gerät; zuerst die Ursache prüfen.

## 9. Wenn etwas nicht klappt

- **Das Einrichtungs-WLAN kommt wieder:** Der Routerzugang wurde nicht erreicht. Öffne http://192.168.4.1 und prüfe WLAN-Name und Kennwort.
- **Die Webseite lädt nicht:** Ermittle die IP-Adresse im Router. PC/Handy und ESP müssen im selben Netzwerk sein.
- **Ein Relais schaltet falsch:** Sende nicht. Prüfe GPIO, LOW-/HIGH-Logik und Verdrahtung.
- **Nach dem Einschalten ist ein Ausgang aktiv:** Trenne die Relaisversorgung, bis Pin und Logik sicher geprüft sind.
- **Ein Update meldet einen Fehler:** Lies die Fehlermeldung ab. Kein Factory-Reset und kein Dateisystem-Upload als Reparaturversuch.

Unter **Diagnose** steht das Formular **Fehler oder Wunsch melden** direkt am Anfang. Du kannst freiwillig einen Fehler, einen Wunsch oder einen sonstigen Hinweis mitteilen. Kontrolliere den Text; Name und Rückkontakt sind freiwillig. Es werden keine Anlagenkonfigurationen, WLAN-Daten oder Sicherungen angehängt. Der Versand läuft ohne E-Mail-Programm über eine sichere HTTPS-Vorschau; erst nach deiner zweiten Bestätigung wird eine E-Mail verschickt. Wenn der Versand nicht klappt, kannst du den Bericht als Datei herunterladen.

## Wörter kurz erklärt

- **GPIO:** Nummer eines ESP-Pins, der zum Beispiel ein Relais steuert.
- **Relais:** Elektrischer Schalter auf der Relaiskarte.
- **Funktion:** Benannte Schalthandlung, zum Beispiel „Antenne 1“.
- **Anlagenteil:** Echtes Gerät wie Funkgerät, PA oder Antenne.
- **Signalweg:** Erlaubte Reihenfolge vom Funkgerät bis zur Antenne.
- **Master/Slave:** Der Master koordiniert den Verbund. Jeder Slave steuert seine eigenen lokalen Ausgänge.

