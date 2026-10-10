# Hinweise zu Drittanbieter-Software

Die GNU GPL in `LICENSE` gilt für die ursprünglichen Inhalte dieses Projekts. Sie ersetzt keine Lizenz eines eingebundenen oder mitgelieferten Drittanbieter-Bestandteils. Die verwendeten Versionen sind in `platformio.ini` festgelegt.

| Bestandteil | Version im Build | Lizenz / Hinweis | Quelle |
|---|---:|---|---|
| Arduino-ESP32-Core von Espressif | 2.0.17 (PlatformIO-Paket `3.20017.241212+sha.dcc1105b`) | LGPL-2.1-or-later; eigener Lizenztext liegt im Upstream-Projekt | [Quellstand und Lizenz](https://github.com/espressif/arduino-esp32/tree/2.0.17) |
| ArduinoJson | 7.4.3 | MIT | [Upstream-Projekt](https://github.com/bblanchon/ArduinoJson/tree/7.4.3) |
| PlatformIO Espressif32-Plattform | 6.12.0 | Apache-2.0 für die Build-Plattform; Build-Werkzeug, nicht eigenständiger Bestandteil der Oberfläche | [Upstream-Projekt](https://github.com/platformio/platform-espressif32/tree/v6.12.0) |
| Terser | 5.44.1 | BSD-2-Clause; Build-Werkzeug zum Komprimieren der Weboberfläche, nicht zur Laufzeit enthalten | [Upstream-Projekt](https://github.com/terser/terser/tree/v5.44.1) |

Der Arduino-Core wird in der Firmware verwendet und bleibt unter seiner LGPL-Lizenz. PlatformIO lädt die festgelegten Quellpakete beim Build; sie werden nicht als separate Bibliotheksordner in diesem Repository gespiegelt. Vor einer Weitergabe geänderter oder zusätzlicher Abhängigkeiten muss diese Liste anhand des konkreten Builds aktualisiert werden.

## Lizenzumfang und kommerzielle Nutzung

Die Projektdateien stehen unter GPL-3.0-or-later. Diese freie Lizenz erlaubt auch kommerzielle Nutzung und Weitergabe, wenn ihre Bedingungen eingehalten werden. Wer eine abweichende Lizenz, kommerzielle Sondervereinbarung oder bezahlte Unterstützung anfragen möchte, kann den Projektkontakt anschreiben. Drittanbieter-Lizenzen bleiben davon unberührt.
