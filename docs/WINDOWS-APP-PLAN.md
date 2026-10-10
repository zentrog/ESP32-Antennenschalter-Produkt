# Geplantes Vorhaben: Windows-App für ESP32-Antennenschalter

**Status: GEPLANT – die gemeinsame Meldeanforderung ist teilweise in der Firmware umgesetzt; die Windows-App selbst ist nicht implementiert.**

Dieses Dokument hält Ziel, Funktionsumfang, Sicherheitsregeln, technische Abhängigkeiten und Freigabekriterien für eine native Windows-Anwendung fest. Es ist die dauerhafte Arbeitsgrundlage für spätere Umsetzung. Eine hier beschriebene Funktion gilt erst als vorhanden, wenn sie gebaut, geprüft und in diesem Dokument sowie den übrigen Projektdokumenten als umgesetzt gekennzeichnet wurde.

## 1. Ziel

Eine herunterladbare Windows-Anwendung soll auch Menschen ohne ESP-, Netzwerk- oder Firmwarekenntnisse sicher durch Einrichtung, Diagnose, Sicherung, Update und Wiederherstellung führen. Im normalen Gebrauch sollen keine Entwicklungsumgebungen oder Programmiersprachen installiert werden müssen. Ein fehlender USB-Treiber kann eine manuelle Installation erfordern; die App muss dann genau erklären, was fehlt und wie es von einer offiziellen Quelle installiert wird.

App und ESP-Firmware bleiben eigenständig nutzbare und separat gepflegte Produktbestandteile. Die bisherige Firmware wird weiterhin separat gebaut, als direkt herunterladbares ESP-Image veröffentlicht und mit einer eigenen Schritt-für-Schritt-Anleitung gepflegt. Anwender, die wissen, was sie tun, können Firmware weiterhin ohne Windows-App manuell installieren oder aktualisieren. Die App ist ein zusätzlicher Komfort- und Diagnoseweg und darf die Firmwarepflege oder die manuellen Download-/Updatewege niemals ersetzen.

Firmware, App, Konfigurationsformat, Updatepakete, Sicherungen und Anleitungen werden trotzdem als zusammengehöriges Produkt abgestimmt. Die App darf keine Fähigkeiten voraussetzen, die die installierte Firmware nicht eindeutig meldet und unterstützt. Die getrennten Downloadartefakte müssen klar bezeichnet und jeweils eigenständig nutzbar sein.

## 2. Produktregeln

1. **Ein geführter Ablauf:** Fachbegriffe werden vermieden oder unmittelbar erklärt. Jeder Schritt nennt Gerät, Wirkung und nächsten Schritt.
2. **Vor jeder Änderung erkennen und bestätigen:** Gerät, Gerätekennung, Rolle, Verbindung und installierte Version werden angezeigt. Der Nutzer bestätigt das Zielgerät.
3. **Erst sichern, dann ändern:** Sicherungen werden vor Update, Rückstufung, Reparatur oder Löschen erstellt, geprüft und dem jeweiligen ESP zugeordnet.
4. **Update und Löschen klar trennen:** Firmwareupdate erhält Konfiguration. Vollständige Neuinstallation löscht sie. Kein mehrdeutiger Ein-Klick-Befehl darf beide Vorgänge vermischen.
5. **Diagnose zunächst nur lesend:** Prüfungen schalten keine Relais und bewegen keinen Rotor. Hardwaretests erfordern eine separate Erklärung und Bestätigung.
6. **Keine erfundenen Ergebnisse:** Nicht erreichbare Geräte, nicht prüfbare Sicherungen und offene Hardwarefragen werden als solche angezeigt. Ein erfolgreicher Download oder Build gilt nicht als erfolgreicher ESP-Start.
7. **Lokale Daten bleiben lokal:** Rufzeichen, PLZ, WLAN-Kennwörter, GPIO-Zuordnungen, Anlagenkonfigurationen und Sicherungen werden nicht an GitHub hochgeladen.
8. **Sicherheitsabbruch statt Raten:** Bei falschem Gerätetyp, unpassender Firmware, ungültiger Signatur, beschädigter Sicherung oder nicht erfüllter Voraussetzung wird nicht geschrieben.

## 3. Hauptabläufe

### 3.1 Start und Geräteerkennung

- Startseite mit den Aufgaben **Gerät einrichten**, **Geräte prüfen**, **Firmware aktualisieren** und **Sicherung wiederherstellen**.
- Suche im lokalen Netzwerk nach erreichbaren Controllern. Anzeige von sprechendem Namen, Basis-MAC/Gerätekennung, IP-Adresse, Controller-Rolle, Firmwareversion und Erreichbarkeit.
- Prüfung, ob ein Verbund erkannt wurde. Geräte werden einzeln aufgeführt; ein vermuteter Master oder Slave wird nicht ohne Firmwarebestätigung behauptet.
- Zusätzlich USB-Erkennung: COM-Port, USB-Gerätekennung soweit vorhanden, erkannter ESP-Chip und lesbare Firmware-/Geräteinformationen.
- Vor Eingriffen verständliche Rückfrage: **„Ist dies das Gerät, das du bearbeiten möchtest?“** Anzeigen, an denen es sicher wiedererkannt werden kann.
- MAC/Basis-MAC ist die primäre Zuordnung für Backups. IP und COM-Port sind wechselnde Verbindungsangaben. USB-Kennung und Platinenseriennummer sind optionale Zusatzmerkmale, keine Voraussetzung.

### 3.2 Netzwerk- und WLAN-Ersteinrichtung

Ein per LAN angeschlossener PC kann ESPs im selben lokalen Netz finden, aber nicht nach WLANs in der Umgebung suchen. Die Ersteinrichtung bietet daher zwei gleichwertig erklärte Wege:

**USB-Weg (Hauptweg für neue oder gelöschte ESPs):**

1. USB anschließen; COM-Port und ESP erkennen.
2. Zielgerät bestätigen.
3. Firmware frisch installieren oder vorhandene Firmware verwenden.
4. WLAN-Name und Kennwort vom Nutzer eingeben lassen und per unterstützter Provisionierung zum ESP übertragen.
5. ESP neu starten, im lokalen Netz wiederfinden und Verbindung bestätigen.

**Handy-/Einrichtungs-Hotspot (Alternative):**

1. App zeigt kurze bebilderte Schritte und bei Bedarf einen QR-Code.
2. Nutzer verbindet das Handy mit dem Einrichtungs-WLAN des ESP und trägt auf dessen Einrichtungsseite das Heim-WLAN ein.
3. App erklärt, dass Windows die WLAN-Verbindung des Handys nicht fernsteuern kann.
4. Nach dem Wechsel ins Heimnetz sucht die App den ESP erneut.

Die App bietet keine WLAN-Passwort- oder Routerdaten als Produktvorgabe an. Der Nutzer gibt sie selbst ein. Ein frischer Slave ohne Netzverbindung kann gemeinsame WLAN-Daten nicht über den Master erhalten; er muss zunächst per USB oder Einrichtungs-Hotspot ins Netz gelangen.

### 3.3 Konfigurationsassistent und Prüfung

Der Assistent ist **optional** und überspringbar. Er führt durch sinnvolle Mindestangaben und kennzeichnet optionale Felder. Je nach Anlage kann er abfragen:

- Rufzeichen, PLZ/Standort und Anlagenname
- WLAN-Namen und Kennwörter
- Controller-Namen und Zuordnung zum Verbund
- Platinenprofil, GPIOs, Relaisnamen und Relaislogik
- Funkgeräte, Antennen, Koax-/HF-Schalter, Funktionen und Signalwege
- Layout und weitere Anlagenangaben, soweit von der Firmware unterstützt

Vor dem Übertragen zeigt die App eine verständliche Vorschau: welche Angaben auf welchen ESP gelangen. Sie liest anschließend den Stand wieder aus und prüft die Annahme. Sie erfindet keine Geräte oder GPIO-Zuordnungen und füllt persönliche Angaben nicht mit Beispieldaten.

Konfigurationsregeln kommen aus der Firmware beziehungsweise einem gemeinsam versionierten Regelmodell. Die App prüft mindestens unterstützte Geräte-/API-Version, fehlende Pflichtfelder, doppelte Gerätekennungen, doppelte GPIO-Belegung, ungültige oder riskante Pins, widersprüchliche Relaislogik, unvollständige Signalwege und Regeln für exklusive Schaltgruppen. Regeln wie H/V-Zuordnung dürfen nicht unabhängig in App und Firmware auseinanderlaufen.

### 3.4 Sicherung und Verbundmanifest

- Vor Änderungen Sicherung für **jeden betroffenen ESP einzeln** anlegen.
- Verbundsicherung enthält ein Manifest mit Geräte-MACs, Controller-IDs/-Rollen, Firmware- und Konfigurationsversionen sowie je ESP dessen eigenes Backup.
- Lokale GPIO-/Board-/Relaiszuordnung bleibt node-lokal; gemeinsame Konfiguration wird getrennt gekennzeichnet und darf sie nicht überschreiben.
- Sicherung umfasst, soweit implementiert und lesbar: persönliche Gerätekonfiguration, WLAN-Daten, lokale Controllerkonfiguration, gemeinsame Anlagenkonfiguration und notwendige Wiederherstellungsmetadaten.
- Vor dem Schreiben Schema, Gerätezuordnung, Vollständigkeit, Integrität und Kompatibilität prüfen. Ungültige Sicherung blockiert den Vorgang.
- Für vollständige Verbundsicherung müssen alle erforderlichen Controller erreichbar und einzeln gesichert sein. Fehlende Geräte werden namentlich angezeigt; keine stillschweigende Teil-Sicherung als vollständige Verbundsicherung ausgeben.
- Backup-Dateien enthalten sensible WLAN- und Anlagendaten. Sie bleiben lokal und werden weder protokolliert noch hochgeladen. Verschlüsselung, Kennwortschutz, Schlüsselverwaltung und portable Wiederherstellung sind vor Umsetzung festzulegen.
- Die MAC-Zuordnung schützt vor versehentlicher falscher Wiederherstellung. Wiederherstellung auf Ersatzhardware mit anderer MAC ist als bewusster Sonderfall möglich und muss neue Geräteidentität, Rollen und Konflikte prüfen.
- Vor vollständigem Löschen muss eine gültige Sicherung auf dem PC liegen. Bei jungfräulichem, noch nie konfiguriertem Gerät darf der Nutzer bewusst ohne Konfigurationsbackup fortfahren; dies wird ausdrücklich als „Es gibt noch keine Konfiguration zu sichern“ bezeichnet.

### 3.5 Update, Rückstufung und Neuinstallation

Die Oberfläche bietet getrennte, sprachlich klare Aktionen:

- **Firmware aktualisieren – Einstellungen behalten**: OTA über Netzwerk oder USB, soweit Gerät und Firmware es unterstützen.
- **Ältere Firmware installieren**: gezielte Auswahl eines offiziellen stabilen GitHub-Releases; zusätzliche Bestätigung, Anzeige von Versions- und Konfigurationskompatibilität.
- **ESP vollständig löschen und neu einrichten**: nur per USB; löscht den Flash-Inhalt einschließlich Einstellungen und WLANs, installiert ein passendes Komplettpaket und bietet danach gezielte Wiederherstellung des passenden Backups an.
- **Reparatur versuchen**: zuerst Diagnose und Erklärung; kein pauschales Löschen. Nur klar abgegrenzte, bestätigte Schritte.

GitHub ist die Quelle offizieller Versionen. Die App fragt Releases ab, lädt das passende Artefakt direkt herunter, prüft kryptografische Signatur und Hash und zeigt Version, Gerätetyp, Datum und Änderungen. Ein Hash allein bestätigt keine Herkunft; Signaturprüfung und Veröffentlichungsschlüssel müssen Teil des Releases sein. Ältere Versionen werden nie ungefragt installiert.

Firmware- und App-Downloads bleiben separate GitHub-Artefakte und getrennte Installationswege. Der direkte Firmwaredownload und die bestehende manuelle Installation bleiben auch dann dokumentiert und verfügbar, wenn eine Windows-App veröffentlicht wird. App-Veröffentlichung darf Firmware-Release nicht blockieren; Firmware-Release darf eine App-Version nicht fälschlich als erforderlich darstellen. Ein gemeinsamer Releaseeintrag kann beide Assets verlinken, muss aber jedes Artefakt getrennt ausweisen.

Ein OTA-Abbild (inaktiver Programmslot) und ein USB-Komplettpaket (Bootloader, Partitionstabelle, Anwendung, nötige OTA-Metadaten und passendes Dateisystem) sind verschiedene Artefakte. Sie dürfen nicht verwechselt werden. Ein Factory-Paket muss leere neutrale Produktvorgaben enthalten, keine WLANs oder privaten Konfigurationen.

### 3.6 Fehler- und Wiederherstellungsweg

Wenn ein Netzwerkvorgang nicht klappt, sagt die App:

1. was sie festgestellt hat (zum Beispiel Gerät nicht erreichbar, WLAN-Verbindung fehlgeschlagen oder Versionsabfrage nicht möglich),
2. was dadurch gerade nicht möglich ist,
3. was der Nutzer als Nächstes genau tun soll,
4. wie er erneut prüfen kann.

Bei USB-Problemen prüft die App COM-Port, erkannte USB-Kennung und Antwort des ESP. Erkennt sie den benötigten Treiber zuverlässig, nennt sie Hersteller, genauen Treibernamen, offiziellen Bezugsort und Installationsschritte; anschließend gibt es **Erneut prüfen**. Ist die Erkennung uneindeutig, sagt sie das ausdrücklich und zeigt Gerätekennung/Windows-Hilfe statt einen Treiber zu erraten. Treiber-/Systeminstallationen werden nicht ungefragt gestartet; Administratorbedarf wird vorab genannt.

Bei Schreibabbruch durch Kabel- oder Stromverlust erklärt die App, welcher Schritt erreicht wurde, ob der ESP noch antwortet und wie USB-Wiederherstellung gestartet wird. Nach jedem Flash wartet sie auf Neustart, liest Version und Identität erneut aus und meldet das tatsächliche Ergebnis je Gerät.

### 3.7 Diagnose

Diagnose ist standardmäßig lesend und trennt **Softwarebefund** von **physischer Prüfung**.

Einzelgerät-Prüfungen:

- Identität, Erreichbarkeit, Firmware/API-/Konfigurationsversion und Speicherstatus
- Pflichtangaben, Konfigurationsintegrität und Wiederherstellbarkeit
- GPIO-/Relaiskonflikte und bekannte Pinrisiken anhand des Platinenprofils
- Vollständigkeit und Widerspruchsfreiheit der Signalwege und Exklusivregeln
- erreichbare Sicherung sowie Update-Kompatibilität

Verbund-Prüfungen:

- erwartete und erreichbare ESPs; fehlende Geräte klar benennen
- eindeutige MACs, Rollen und Master-/Slave-Zuordnung
- Firmware/API-/Konfigurations-Kompatibilität aller Knoten
- lokale Sicherung jedes Knotens und gemeinsamer Konfigurationsstand
- gemeinsame Routen über beteiligte Geräte und unerreichbare Schaltstellen
- Versionsabweichung und vorgeschlagene sichere Aktualisierungsreihenfolge

Ergebnisse lauten verständlich etwa **„Geprüft – kein Softwarekonflikt gefunden“**, **„Hinweis – Controller 2 ist nicht erreichbar“** oder **„Problem – GPIO 18 ist doppelt belegt“**. Die App weist darauf hin: Sie kann Softwarezustand und Rückmeldungen prüfen, aber weder Kabel noch Relais, Stromversorgung, Antenne oder Rotor mechanisch sehen. Hardwaretests schalten nur nach eigener Warnung und Zustimmung.

### 3.8 Desktop-Verknüpfung

Nach erfolgreicher Einrichtung fragt die App, ob sie eine Desktop-Verknüpfung anlegen soll. Eine fest eingetragene IP kann sich ändern. Bevorzugt wird eine Verknüpfung, die den Controller im lokalen Netz erneut sucht; alternativ wird eine Router-reservierte IP beziehungsweise ein funktionierender lokaler Hostname verwendet. Die App erklärt den Unterschied einfach und legt nichts ohne Zustimmung an.

### 3.9 Fehler melden (Firmware und Windows-App)

In der Firmware-Weboberfläche und in der Windows-App gibt es unter **Diagnose** die gut sichtbare Aktion **Fehler oder Wunsch melden**. Der Nutzer wählt **Fehler**, **Verbesserungsidee/Wunsch** oder **Sonstiges**. Der geführte Fehlerteil fragt: Was ist passiert? Was wurde erwartet? Bei welchem Schritt trat es auf? Seit wann beziehungsweise nach welcher Änderung? Zusätzlich gibt es ein freies Textfeld für selbst erkannte Fehler, Wünsche, Verbesserungsvorschläge und weitere Hinweise.

Name und Rückkontakt sind optionale, getrennte Felder. Der Nutzer kann etwa Namen, Rufzeichen und E-Mail-Adresse angeben, damit eine Rückfrage möglich ist. Diese Angaben werden nicht automatisch aus der ESP-Konfiguration oder dem PayPal-Profil übernommen. Vor dem Teilen zeigt die App ausdrücklich, ob Name, Rückkontakt und Freitext im Bericht enthalten sind; jedes Feld kann einzeln entfernt werden. Ohne Zustimmung bleiben Bericht und Angaben lokal.

- Beide Oberflächen erzeugen ein gemeinsames, versioniertes Diagnosebericht-Format. Die Firmware kann einen Bericht im Browser zum Herunterladen bereitstellen; die App kann Berichte von einem oder mehreren erreichbaren ESPs zusammenstellen.
- Der Bericht enthält standardmäßig nur für die Fehlersuche nötige technische Angaben: Firmware-/App-/API-Version, ESP-Modell, anonymisierte Gerätezahl/Rollen, Betriebssystem- und USB-Erkennung soweit relevant, Fehlercodes, Ergebnis der Grunddiagnose und einen begrenzten, geheimnisbereinigten Ereignisausschnitt.
- Standardmäßig ausgeschlossen sind WLAN-Kennwörter, WLAN-Namen, Rufzeichen, PLZ, private IP-Adressen, MAC-Adressen, vollständige Anlagen-/GPIO-Konfigurationen und Backups. Freitext, Name und Rückkontakt werden nur nach eigener Eingabe des Nutzers aufgenommen.
- Vor dem Teilen zeigt die App/Firmware eine vollständige Vorschau und weist darauf hin, dass selbst eingegebener Text private Daten oder Zugangsdaten enthalten kann. Name, Kontakt und Beschreibung lassen sich vor dem Erstellen einzeln entfernen oder ändern. Eine automatische Erkennung möglicher Geheimnisse darf nur warnen und ersetzt nicht die Nutzerprüfung.
- **HTTPS ist nur der sichere Transport und kein E-Mail-Provider.** Für den Versand wird ein serverseitiger Mail-Endpunkt benötigt: App/Firmware sendet den bestätigten Bericht per HTTPS an diesen Endpunkt; der Endpunkt validiert und begrenzt die Anfrage und reicht die E-Mail anschließend über authentifiziertes SMTP (Submission) oder die API eines E-Mail-Versanddienstes an den Projektkontakt weiter. Erst dieser letzte Dienst stellt die E-Mail zu.
- Der bevorzugte Ablauf ist eine ausdrücklich vom Nutzer gestartete **„Bericht prüfen und per E-Mail senden“**-Aktion. Dafür braucht der Nutzer kein eingerichtetes E-Mail-Programm. Nach Vorschau und ausdrücklicher Zustimmung übermittelt App/Firmware den gewählten Bericht an den Mail-Endpunkt und zeigt eine Versandbestätigung oder verständliche Fehlermeldung.
- Als Absender ist ein eigens für den ESP32-Projektversand angelegtes Mailkonto vorgesehen; Zieladresse ist der freigegebene Projektkontakt. Die Zugangsdaten liegen ausschließlich als geschützte Geheimnisse auf dem Mailserver. Sie dürfen niemals in Firmware, App, GitHub-Quelltext, Release-Dateien, Logs oder Diagnoseberichte eingebettet werden. Auch ein nur zum Mailversand angelegtes Konto bleibt ein zu schützendes Zugangsmittel.
- Der aktuelle Firmware-Arbeitsstand enthält das Formular, den lokalen JSON-Download und die Übergabe an eine HTTPS-Vorschauseite. Details und Hosting-Freigabekriterien stehen in [REPORT-ENDPOINT.md](REPORT-ENDPOINT.md). Solange der do1anb.de-Endpunkt nicht auf echtem Hosting geprüft ist, ist der E-Mail-Versand ein offener Freigabepunkt.
- Ein HTTPS-Endpunkt ohne konfigurierten SMTP-/API-Versand kann keine E-Mail zustellen. Gibt es keinen erreichbaren und eingerichteten Backend-Mailversand, bleiben nur ein vorhandenes E-Mail-Programm, ein manueller Webmailweg oder das lokale Speichern und spätere Teilen des Berichts.
- Die Firmware-Weboberfläche bietet denselben Ablauf, soweit sichere HTTPS-Übertragung vom Browser aus möglich ist. Da die ESP-Webseite derzeit lokal über HTTP laufen kann, muss die Umsetzung den Übertragungsweg und die Integrität der angezeigten Einwilligung prüfen. Wenn kein sicherer Versand möglich ist, darf sie nicht still auf unsichere Übertragung ausweichen; sie bietet dann lokalen Download und vorbereitete E-Mail als Alternative.
- E-Mail-Programm und GitHub bleiben optionale Wege. Ein `mailto:` allein ist kein verlässlicher Hauptweg, weil ein E-Mail-Programm fehlen oder nicht eingerichtet sein kann. Die App darf alternativ eine vorbereitete E-Mail oder GitHub-Issue-Seite öffnen; das Teilen erfolgt erst nach Prüfung und eigener Aktion des Nutzers.
- Mail-Endpunkt und Mailprovider sind getrennte Komponenten und müssen beide eingerichtet, überwacht und gegen Missbrauch abgesichert sein. Es braucht Größenlimits, Ratenbegrenzung, Empfängerprüfung, transparente Verarbeitung und eine klare Fehlermeldung bei Zustellfehlern. Der Endpunkt speichert keine vollständigen Berichte dauerhaft; unvermeidbare technische Protokolle und ihre Aufbewahrungszeit müssen vor Start festgelegt und im Datenschutzhinweis erklärt werden.
- Lokaler Download bleibt immer als Ausweichweg möglich; bei fehlender Internetverbindung kann der Nutzer den Bericht später senden. Fehlerbericht ist nie Voraussetzung für Update, Wiederherstellung oder normalen Betrieb.
- Ein Bericht hat Größen- und Zeitgrenzen, Versionsfeld, Erstellungsdatum, Zufallsfallnummer und eine verständliche Kurzfassung. Keine unbegrenzten seriellen Logs oder kompletten Flash-/Konfigurationsdumps anhängen.
- Kann die Software eine Ursache nicht feststellen oder Hardware nicht sehen, muss der Bericht genau das sagen. Kein automatisch formulierter Bericht darf aus einem Softwarehinweis eine bestätigte defekte Hardware ableiten.
- Fehlerberichte dürfen nie Voraussetzung für Update, Wiederherstellung oder normalen Betrieb sein.

## 4. Windows-Unterstützung und Paketierung

- Gewünschter Zielbereich: Windows 7 SP1 bis Windows 11; 32-/64-Bit-Abdeckung und Mindestupdates müssen festgelegt werden.
- Primärziel ist eine herunterladbare, möglichst portable App ohne vorherige Python-, PlatformIO-, VS-Code- oder Entwicklerlaufzeitinstallation.
- Fehlende USB-Treiber dürfen einen manuellen Installationsschritt erfordern; App erkennt und beschreibt ihn nachvollziehbar.
- Vor Wahl von Sprache/GUI-Framework ist ein kleiner Windows-7-SP1-Kompatibilitätsnachweis erforderlich. Moderne Laufzeiten unterstützen Windows 7 nicht mehr; eine ältere oder native Implementierung bedeutet höhere Wartungs- und Sicherheitskosten.
- Netzwerkzugriff (GitHub HTTPS/TLS), Zertifikate, Windows-Firewall, Antivirus/SmartScreen, UAC und fehlende Administratorrechte sind eigene Prüfungen. Nicht erfüllte Voraussetzung wird mit konkretem nächsten Schritt statt generischem Fehler angezeigt.
- Unterstützte Betriebssysteme werden ehrlich nach tatsächlich bestandener Prüfung ausgewiesen. Wenn eine Funktion unter Windows 7 wegen TLS/Komponenten nicht sicher verfügbar ist, wird ein sicherer Fallback genannt oder der Vorgang gestoppt.
- Installer/portable Paket, Signierung des Programms und Updates des Flasher-Programms gehören zum Release- und Vertrauensmodell.

## 5. Zusammenarbeit von App und Firmware

Vor App-Implementierung ist ein versionierter Gerätevertrag erforderlich:

- Geräteidentität, Modell, Rolle, Versionen und Fähigkeiten
- USB-Erkennung und optionales Provisionierungsprotokoll
- Netzwerk-Erkennung sowie Status-/Diagnoseantwort
- Konfiguration exportieren/importieren, Backup-Schema und Integritätsprüfung
- gemeinsame gegenüber lokalen Konfigurationsfeldern
- Update- und Wiederherstellungszustände, Fehlercodes und bestätigter Neustart
- gemeinsames Diagnosebericht-Schema, Fehlercodes und definierte Geheimnisbereinigung
- Versionsverhandlung, Schema-Migration und zulässiger Rückschritt
- sichere Aktionen, die Bestätigung verlangen

Der ESP bestätigt nach Import die tatsächlich gespeicherten Daten durch Readback. Ein HTTP-Erfolg oder serielle Empfangsbestätigung allein zählt nicht als angenommene Konfiguration. Sensible Zugangsdaten erscheinen nicht in normalen Logs.

## 6. Öffentliche Hinweise, Lizenz und Datenschutz

- Im Programm: Produktname, Versionsnummer, Copyright-/Urheberangabe, Projektkontakt, Lizenz und Links zu Anleitung sowie Datenschutz-/Nutzungshinweisen.
- Vom Betreiber zur öffentlichen Projektangabe freigegeben: **Andreas Bodyn (DO1ANB)**, **andreas.bodyn@gmx.de**; vorgesehene Kennzeichnung: **© 2026 Andreas Bodyn (DO1ANB)**. Diese Kontaktangaben dürfen in README, App-Info und Projekt-Footer erscheinen. Private Anlagen-PLZ, WLAN-Daten und Konfigurationen bleiben davon getrennt und werden nicht veröffentlicht.
- Optionale freiwillige Projektunterstützung über [PayPal.Me](https://paypal.me/andreasbodyn): In der Windows-App gibt es einen klar als freiwillig bezeichneten Unterstützungsbutton; in der Firmware erscheint derselbe dezente Button im Footer; die Projekt-/Downloadseite verlinkt ebenfalls direkt auf PayPal.Me. Ein Klick öffnet ausschließlich den PayPal.Me-Link im Browser. Es gibt keine eingebettete Zahlung und keine automatische Weiterleitung.
- Unterstützung darf niemals Voraussetzung für Download, Installation, Updates, Funktionen oder Support sein. Ein Beitrag begründet keinen Supportanspruch. Copyright, Projektkontakt und Bedienfunktionen bleiben sichtbar und werden durch den Button nicht verdrängt.
- Im GitHub-Release: Quellcommit, Änderungsübersicht, unterstützte ESP-Modelle/Windows-Versionen, Dateityp je Firmwarepaket, Hash/Signatur und bekannte Grenzen.
- Drittanbieterbibliotheken, Firmwarekomponenten und Flasher müssen mit ihren Lizenztexten/NOTICE-Dateien ausgeliefert werden. App-, Firmware- und Dokumentationslizenz dürfen nicht ungeprüft gleichgesetzt werden.
- Test-/Vorabversionen werden eindeutig gekennzeichnet; stabile und experimentelle Releases nicht verwechseln.
- Hinweise zu Funk-/Antennenanlage, möglicher Relais-/Rotorbewegung, Backup-Sensibilität und physischer Prüfung sind verständlich und sichtbar.
- Keine pauschale Aussage „jede Gewährleistung ausgeschlossen“ ohne rechtliche Prüfung. Lizenz-, Haftungs-, Gewährleistungs- und Nutzungsbedingungen sind vor öffentlicher Freigabe für die konkrete Verteilung rechtlich zu prüfen.

## 7. Geplanter Umsetzungsplan

1. **Bestand aufnehmen:** aktuelle ESP-Identität, API, WLAN-Provisionierung, Backupformat, Verbundzustände, Partitionen und Release-Artefakte dokumentieren.
2. **Gerätevertrag festlegen:** Fähigkeiten, API-/Backup-Schema und Fehlercodes zwischen ESP und App entwerfen; lokale und gemeinsame Einstellungen trennen.
3. **Backup/Wiederherstellung in Firmware:** Export, Prüfung, Import, Readback und Identitätsabgleich zuerst implementieren und an Einzelgerät und Verbund verifizieren.
4. **Factory- und OTA-Artefakte:** eindeutig benannte, signierte Pakete mit Manifest, Modell, Version, Flashadressen und Hash bauen.
5. **Windows-Kompatibilitätsprobe:** Windows 7 SP1, Windows 10 und Windows 11 mit USB-Brücken, TLS/GitHub, COM-Erkennung und fehlenden Treibern untersuchen; danach GUI-Framework/Paketierung entscheiden.
6. **App-Grundfunktionen:** Erkennung, Bestätigung, Sicherung, Update und Ergebnisbericht bauen.
7. **Einrichtung, Diagnose und Fehlerberichte:** Konfigurationsassistent, USB-/Handy-Provisionierung, Einzel-/Verbunddiagnose, gemeinsames anonymisiertes Fehlerbericht-Format, Vorschau und optionale Desktop-Verknüpfung ergänzen.
8. **Verbund- und Rettungsprüfungen:** fehlender Slave, wechselnde IP, falsches Backup, ältere Version, fehlgeschlagener Download, Abbruch beim Schreiben und Wiederherstellung nach Factory-Install abdecken.
9. **Dokumentieren und veröffentlichen:** signiertes App-Paket, passende Firmware, Prüfprotokoll, einfache Anleitung, Lizenz-/Drittherstellerhinweise und Release-Notizen synchron aus einem geprüften Commit veröffentlichen.

Kein Punkt wird als abgeschlossen markiert, bevor konkrete Ergebnisse und getestete Umgebungen dokumentiert sind. Simulator-/Build-Ergebnis, USB-Hardwaretest, Netzwerk-Update und reale Verbundwiederherstellung werden getrennt ausgewiesen.

## 8. Freigabekriterien

Eine erste öffentliche Version ist erst freigabefähig, wenn:

- ein Anwender ohne Fachwissen ESP und Zielaktion zuverlässig identifizieren kann;
- Firmware-Update Konfiguration erhält und vollständiges Löschen sichtbar als destruktiv bestätigt wird;
- vor Änderung alle betroffenen Geräte gültig und passend gesichert sind;
- Sicherung für denselben ESP korrekt wiederhergestellt und durch Readback bestätigt wurde;
- Verbundmanifest alle erwarteten und fehlenden Geräte ausweist und Einzelbackups korrekt zuordnet;
- Firmware-, Konfigurations- und Betriebssystem-Inkompatibilitäten sicher stoppen;
- Signatur/Hash, Version, Modell und Flashlayout jedes Pakets geprüft werden;
- Strom-/USB-Abbruch einen dokumentierten und praktisch geprüften Rettungsweg besitzt;
- Firmware-Weboberfläche und App erzeugen denselben versionierten Bericht, der standardmäßig keine WLAN-, Identitäts-, Standort- oder Anlagengeheimnisse enthält;
- eigene Fehlerbeschreibung, Wünsche/Verbesserungsideen sowie Name und Rückkontakt können freiwillig eingegeben, einzeln entfernt und vor dem Teilen vollständig geprüft werden;
- Berichtvorschau, lokaler Download und bewusster Versand funktionieren auch ohne lokales E-Mail-Programm; kein Bericht wird ohne ausdrückliche Bestätigung versandt;
- der HTTPS-Mail-Endpunkt leitet Berichte tatsächlich über einen eingerichteten SMTP-/E-Mail-API-Provider weiter, speichert keine vollständigen Berichte dauerhaft, schützt alle Zugangsdaten serverseitig und zeigt Zustellerfolg oder einen brauchbaren Fehler;
- Diagnosen keine unerwarteten Relais-/Motoraktionen auslösen;
- Datenschutz, Lizenz, Copyright, Drittanbieterhinweise und verständliche Anleitung vollständig sind;
- der freiwillige Unterstützungsbutton in App und Firmware sowie der Link auf der Projektseite ausschließlich den bestätigten PayPal.Me-Link öffnen und keine Funktion oder Unterstützung an eine Zahlung knüpfen;
- die tatsächlich unterstützten Windows- und ESP-Versionen auf realen Systemen geprüft und genannt sind.

## 9. Offene Entscheidungen

- Welche Windows-Versionen und Architekturen werden verbindlich unterstützt (insbesondere Windows 7 SP1 und 32-Bit)?
- Welche Sprache/GUI-Technik erfüllt Win7, moderne Windows-Versionen, Portable-App und langfristige Sicherheitsupdates mit vertretbarem Aufwand?
- Wie wird ein portables Backup mit WLAN-Passwörtern verschlüsselt und auf einem Ersatz-PC wiederhergestellt?
- Welche Teile der Flashspeicherung können per USB sicher ausgelesen werden, und welche Konfigurationsbereiche werden ausschließlich logisch exportiert?
- Welche signierte Metadaten-/Schlüsselstrategie schützt Releases und wie werden Schlüsselwechsel/revokierte Releases behandelt?
- Welches Provisionierungsprotokoll wird über USB genutzt; unterstützt Firmware bereits einen sicheren Einrichtungs-Hotspot und QR-Code?
- Welche Wiederherstellung auf ESP-Ersatzhardware mit anderer MAC ist zulässig und wie werden neue Identitäten verteilt?
- Welche Angaben sind wirklich zwingend, welche optional, und welche Prüfregeln sind pro Anlagenprofil konfigurierbar?
- Wo wird der HTTPS-Mail-Endpunkt betrieben, lässt der ausgewählte Hosting-Anbieter authentifizierten SMTP-Versand zu, wie werden Zugangsdaten serverseitig gespeichert/rotiert, welche Minimaldaten verarbeitet der Weg, wie wird Missbrauch begrenzt und wie lange bleiben technische Versandprotokolle erhalten?
- Die vom Betreiber gewählte Projektlizenz ist GNU GPL Version 3 oder später. Vor der App-Veröffentlichung sind Drittanbieter-Lizenzen getrennt zu erfassen; App-eigene Signaturschlüssel sind geschützt zu verwalten.

## 10. Umsetzungshistorie

Noch keine App-Funktion umgesetzt. Bei Beginn jeder Phase Datum, Commit, betroffene Firmware/API-/Backupversion, Ergebnisse, offene Fehler und getestete Windows-/ESP-Geräte ergänzen. Firmware- und App-Releases müssen in `docs/VALIDATION.md` auf dieselbe konkrete Quellrevision zurückgeführt werden.
