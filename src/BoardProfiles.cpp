#include "BoardProfiles.h"
#include <driver/gpio.h>

static String noteFor(const BoardPin& p,const String& lang){return lang=="en"?p.noteEn:p.noteDe;}

static BoardDefinition make30() {
  BoardDefinition b;
  b.id="devkit-v1-30";
  b.nameDe="ESP32-WROOM-32 · 30-Pin-Anschlussleistenprofil";
  b.nameEn="ESP32-WROOM-32 · 30-pin header profile";
  b.noteDe="Dieses Profil beschreibt exakt die hier dargestellte 30-polige Anschlussleisten-Reihenfolge einer Trägerplatine mit ESP32-WROOM-32. Es legt keinen Hersteller der Trägerplatine fest. GPIO-Eigenschaften und Sperren stammen aus der Espressif-Dokumentation des klassischen ESP32 und des ESP32-WROOM-32.";
  b.noteEn="This profile describes exactly the displayed 30-pin header order of a carrier board using ESP32-WROOM-32. It does not identify a carrier-board manufacturer. GPIO capabilities and restrictions are based on Espressif documentation for the classic ESP32 and ESP32-WROOM-32.";
  b.pins = {
    {"VIN",-1,PinClass::Power,"Versorgungspin der 30-poligen Trägerplatine mit Aufdruck VIN. Kein ESP32-GPIO. Die elektrische Funktion des VIN-Pins gehört zur Trägerplatine und ist ohne deren exakte Hersteller-/Schaltplanidentifikation nicht als 5-V- oder Eingang-/Ausgangspfad festgelegt.","Supply pin of the 30-pin carrier board labelled VIN. Not an ESP32 GPIO. The electrical VIN function belongs to the carrier board and is not identified as a 5 V/input/output path without the exact carrier-board manufacturer/schematic.","left",0},
    {"GND",-1,PinClass::Ground,"Masse / 0 V.","Ground / 0 V.","left",1},
    {"GPIO13",13,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH4, TOUCH4, JTAG MTCK. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH4, TOUCH4, JTAG MTCK. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",2},
    {"GPIO12",12,PinClass::Caution,"Boot-Strapping-Pin MTDI. Ein Relaismodul kann beim Reset den Pegel beeinflussen; insbesondere GPIO12 kann dadurch die Flash-Versorgungsauswahl verändern. Nur verwenden, wenn der ESP trotz angeschlossenem Relais sicher startet.","MTDI boot strapping pin. A relay module can affect its reset level; GPIO12 in particular can change flash supply selection. Use only if the ESP reliably boots with the relay connected.","left",3},
    {"GPIO14",14,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH6, TOUCH6, JTAG MTMS. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH6, TOUCH6, JTAG MTMS. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",4},
    {"GPIO27",27,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH7, TOUCH7. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH7, TOUCH7. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",5},
    {"GPIO26",26,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH9 und DAC2. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH9 and DAC2. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",6},
    {"GPIO25",25,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH8 und DAC1. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH8 and DAC1. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",7},
    {"GPIO33",33,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC1_CH5, TOUCH8, XTAL_32K_N. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC1_CH5, TOUCH8, XTAL_32K_N. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",8},
    {"GPIO32",32,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC1_CH4, TOUCH9, XTAL_32K_P. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC1_CH4, TOUCH9, XTAL_32K_P. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","left",9},
    {"GPIO35",35,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH7. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH7. No output driver. No internal pull-up/pull-down resistors.","left",10},
    {"GPIO34",34,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH6. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH6. No output driver. No internal pull-up/pull-down resistors.","left",11},
    {"VN/GPIO39",39,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH3 / S_VN. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH3 / S_VN. No output driver. No internal pull-up/pull-down resistors.","left",12},
    {"VP/GPIO36",36,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH0 / S_VP. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH0 / S_VP. No output driver. No internal pull-up/pull-down resistors.","left",13},
    {"EN",-1,PinClass::Control,"CHIP_PU / Reset. LOW setzt den ESP32 zurück. Kein GPIO für Relaissteuerung.","CHIP_PU / reset. LOW resets the ESP32. Not a GPIO for relay control.","left",14},

    {"3V3",-1,PinClass::Power,"3,3-V-Versorgungsschiene. Kein GPIO.","3.3 V supply rail. Not a GPIO.","right",0},
    {"GND",-1,PinClass::Ground,"Masse / 0 V.","Ground / 0 V.","right",1},
    {"GPIO15",15,PinClass::Caution,"Boot-Strapping-Pin MTDO. Die externe Relaisbeschaltung kann beim Reset Boot-Ausgaben beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","MTDO boot strapping pin. External relay circuitry can affect boot output at reset. Use only if startup and operation are reliable with the relay connected.","right",2},
    {"GPIO2",2,PinClass::Caution,"Boot-Strapping-Pin. Die externe Relaisbeschaltung kann beim Reset den Bootmodus beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","Boot strapping pin. External relay circuitry can affect boot mode at reset. Use only if startup and operation are reliable with the relay connected.","right",3},
    {"GPIO4",4,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH0, TOUCH0. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH0, TOUCH0. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",4},
    {"GPIO16",16,PinClass::Recommended,"Digitaler Ein-/Ausgang des ESP32-WROOM-32; U2RXD ist eine IO-MUX-Funktion. In diesem WROOM-32-Profil als Relais-Steuerausgang freigegeben.","Digital input/output on ESP32-WROOM-32; U2RXD is an IO-MUX function. Enabled as a relay-control output in this WROOM-32 profile.","right",5},
    {"GPIO17",17,PinClass::Recommended,"Digitaler Ein-/Ausgang des ESP32-WROOM-32; U2TXD ist eine IO-MUX-Funktion. In diesem WROOM-32-Profil als Relais-Steuerausgang freigegeben.","Digital input/output on ESP32-WROOM-32; U2TXD is an IO-MUX function. Enabled as a relay-control output in this WROOM-32 profile.","right",6},
    {"GPIO5",5,PinClass::Caution,"Boot-Strapping-Pin. Die externe Relaisbeschaltung kann beim Reset den Pegel beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","Boot strapping pin. External relay circuitry can affect its reset level. Use only if startup and operation are reliable with the relay connected.","right",7},
    {"GPIO18",18,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktion VSPICLK. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX function VSPICLK. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",8},
    {"GPIO19",19,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktionen U0CTS, VSPIQ und EMAC_TXD0. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX functions U0CTS, VSPIQ and EMAC_TXD0. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",9},
    {"GPIO21",21,PinClass::Recommended,"Digitaler Ein-/Ausgang. Arduino-ESP32 Generic verwendet GPIO21 als Standard-SDA für I²C. ESP32-IO-MUX: GPIO21, VSPIHD, EMAC_TX_EN. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. Arduino-ESP32 Generic uses GPIO21 as the default I²C SDA pin. ESP32 IO-MUX: GPIO21, VSPIHD, EMAC_TX_EN. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",10},
    {"GPIO3/RX0",3,PinClass::Forbidden,"UART0-RX. Diese Firmware verwendet UART0 für Flashen und seriellen Monitor. Deshalb als Relaisausgang gesperrt.","UART0 RX. This firmware uses UART0 for flashing and the serial monitor. Therefore blocked as a relay output.","right",11},
    {"GPIO1/TX0",1,PinClass::Forbidden,"UART0-TX. Diese Firmware verwendet UART0 für Boot-/Diagnoseausgaben und den seriellen Monitor. Deshalb als Relaisausgang gesperrt.","UART0 TX. This firmware uses UART0 for boot/diagnostic output and the serial monitor. Therefore blocked as a relay output.","right",12},
    {"GPIO22",22,PinClass::Recommended,"Digitaler Ein-/Ausgang. Arduino-ESP32 Generic verwendet GPIO22 als Standard-SCL für I²C. ESP32-IO-MUX: GPIO22, U0RTS, VSPIWP, EMAC_TXD1. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. Arduino-ESP32 Generic uses GPIO22 as the default I²C SCL pin. ESP32 IO-MUX: GPIO22, U0RTS, VSPIWP, EMAC_TXD1. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",13},
    {"GPIO23",23,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktion VSPID. Interne Pull-up/Pull-down-Widerstände vorhanden. In diesem Steuergerät als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX function VSPID. Internal pull-up/pull-down resistors available. Enabled as a relay-control output in this controller.","right",14},
  };
  return b;
}

static BoardDefinition make38() {
  BoardDefinition b;
  b.id="devkitc-38";
  b.nameDe="Espressif ESP32-DevKitC / WROOM-32 (38 Pins)";
  b.nameEn="Espressif ESP32-DevKitC / WROOM-32 (38 pins)";
  b.noteDe="38-poliges ESP32-DevKitC-Anschlussleistenprofil mit ESP32-WROOM-32. Die Anschlussleisten-Reihenfolge und GPIO-Funktionen entsprechen der Espressif-DevKitC-Dokumentation.";
  b.noteEn="38-pin ESP32-DevKitC header profile with ESP32-WROOM-32. Header order and GPIO functions follow Espressif DevKitC documentation.";
  b.pins = {
    {"3V3",-1,PinClass::Power,"3,3-V-Versorgungsschiene. Kein GPIO.","3.3 V supply rail. Not a GPIO.","left",0},
    {"EN",-1,PinClass::Control,"CHIP_PU / Reset. LOW setzt den ESP32 zurück. Kein GPIO für Relaissteuerung.","CHIP_PU / reset. LOW resets the ESP32. Not a GPIO for relay control.","left",1},
    {"VP/GPIO36",36,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH0 / S_VP. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH0 / S_VP. No output driver. No internal pull-up/pull-down resistors.","left",2},
    {"VN/GPIO39",39,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH3 / S_VN. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH3 / S_VN. No output driver. No internal pull-up/pull-down resistors.","left",3},
    {"GPIO34",34,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH6. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH6. No output driver. No internal pull-up/pull-down resistors.","left",4},
    {"GPIO35",35,PinClass::InputOnly,"Nur digitaler/analoger Eingang; ADC1_CH7. Kein Ausgangstreiber. Keine internen Pull-up/Pull-down-Widerstände.","Digital/analog input only; ADC1_CH7. No output driver. No internal pull-up/pull-down resistors.","left",5},
    {"GPIO32",32,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC1_CH4, TOUCH9, XTAL_32K_P. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC1_CH4, TOUCH9, XTAL_32K_P. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",6},
    {"GPIO33",33,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC1_CH5, TOUCH8, XTAL_32K_N. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC1_CH5, TOUCH8, XTAL_32K_N. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",7},
    {"GPIO25",25,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH8 und DAC1. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH8 and DAC1. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",8},
    {"GPIO26",26,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH9 und DAC2. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH9 and DAC2. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",9},
    {"GPIO27",27,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH7, TOUCH7. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH7, TOUCH7. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",10},
    {"GPIO14",14,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH6, TOUCH6, JTAG MTMS. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH6, TOUCH6, JTAG MTMS. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",11},
    {"GPIO12",12,PinClass::Caution,"Boot-Strapping-Pin MTDI. Ein Relaismodul kann beim Reset den Pegel beeinflussen; insbesondere GPIO12 kann dadurch die Flash-Versorgungsauswahl verändern. Nur verwenden, wenn der ESP trotz angeschlossenem Relais sicher startet.","MTDI boot strapping pin. A relay module can affect its reset level; GPIO12 in particular can change flash supply selection. Use only if the ESP reliably boots with the relay connected.","left",12},
    {"GND",-1,PinClass::Ground,"Masse / 0 V.","Ground / 0 V.","left",13},
    {"GPIO13",13,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH4, TOUCH4, JTAG MTCK. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH4, TOUCH4, JTAG MTCK. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","left",14},
    {"SD2/GPIO9",9,PinClass::Forbidden,"Mit dem SPI-Flash des WROOM-32 verbunden. Nicht als Anwender-GPIO verwenden.","Connected to the WROOM-32 SPI flash. Do not use as an application GPIO.","left",15},
    {"SD3/GPIO10",10,PinClass::Forbidden,"Mit dem SPI-Flash des WROOM-32 verbunden. Nicht als Anwender-GPIO verwenden.","Connected to the WROOM-32 SPI flash. Do not use as an application GPIO.","left",16},
    {"CMD/GPIO11",11,PinClass::Forbidden,"Mit dem SPI-Flash des WROOM-32 verbunden. Nicht als Anwender-GPIO verwenden.","Connected to the WROOM-32 SPI flash. Do not use as an application GPIO.","left",17},
    {"5V",-1,PinClass::Power,"5-V-Versorgungspin des Espressif ESP32-DevKitC. Kein GPIO.","5 V power-supply pin of the Espressif ESP32-DevKitC. Not a GPIO.","left",18},

    {"GND",-1,PinClass::Ground,"Masse / 0 V.","Ground / 0 V.","right",0},
    {"GPIO23",23,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktion VSPID. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX function VSPID. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","right",1},
    {"GPIO22",22,PinClass::Recommended,"Digitaler Ein-/Ausgang. Arduino-ESP32 Generic verwendet GPIO22 als Standard-SCL für I²C. ESP32-IO-MUX: GPIO22, U0RTS, VSPIWP, EMAC_TXD1. Als Relais-Steuerausgang freigegeben.","Digital input/output. Arduino-ESP32 Generic uses GPIO22 as the default I²C SCL pin. ESP32 IO-MUX: GPIO22, U0RTS, VSPIWP, EMAC_TXD1. Enabled as a relay-control output.","right",2},
    {"GPIO1/TX0",1,PinClass::Forbidden,"UART0-TX. Diese Firmware verwendet UART0 für Boot-/Diagnoseausgaben und den seriellen Monitor. Deshalb als Relaisausgang gesperrt.","UART0 TX. This firmware uses UART0 for boot/diagnostic output and the serial monitor. Therefore blocked as a relay output.","right",3},
    {"GPIO3/RX0",3,PinClass::Forbidden,"UART0-RX. Diese Firmware verwendet UART0 für Flashen und seriellen Monitor. Deshalb als Relaisausgang gesperrt.","UART0 RX. This firmware uses UART0 for flashing and the serial monitor. Therefore blocked as a relay output.","right",4},
    {"GPIO21",21,PinClass::Recommended,"Digitaler Ein-/Ausgang. Arduino-ESP32 Generic verwendet GPIO21 als Standard-SDA für I²C. ESP32-IO-MUX: GPIO21, VSPIHD, EMAC_TX_EN. Als Relais-Steuerausgang freigegeben.","Digital input/output. Arduino-ESP32 Generic uses GPIO21 as the default I²C SDA pin. ESP32 IO-MUX: GPIO21, VSPIHD, EMAC_TX_EN. Enabled as a relay-control output.","right",5},
    {"GND",-1,PinClass::Ground,"Masse / 0 V.","Ground / 0 V.","right",6},
    {"GPIO19",19,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktionen U0CTS, VSPIQ und EMAC_TXD0. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX functions U0CTS, VSPIQ and EMAC_TXD0. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","right",7},
    {"GPIO18",18,PinClass::Recommended,"Digitaler Ein-/Ausgang. IO-MUX-Funktion VSPICLK. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. IO-MUX function VSPICLK. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","right",8},
    {"GPIO5",5,PinClass::Caution,"Boot-Strapping-Pin. Die externe Relaisbeschaltung kann beim Reset den Pegel beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","Boot strapping pin. External relay circuitry can affect its reset level. Use only if startup and operation are reliable with the relay connected.","right",9},
    {"GPIO17",17,PinClass::Recommended,"Digitaler Ein-/Ausgang des ESP32-WROOM-32; U2TXD ist eine IO-MUX-Funktion. In diesem WROOM-32-Profil als Relais-Steuerausgang freigegeben.","Digital input/output on ESP32-WROOM-32; U2TXD is an IO-MUX function. Enabled as a relay-control output in this WROOM-32 profile.","right",10},
    {"GPIO16",16,PinClass::Recommended,"Digitaler Ein-/Ausgang des ESP32-WROOM-32; U2RXD ist eine IO-MUX-Funktion. In diesem WROOM-32-Profil als Relais-Steuerausgang freigegeben.","Digital input/output on ESP32-WROOM-32; U2RXD is an IO-MUX function. Enabled as a relay-control output in this WROOM-32 profile.","right",11},
    {"GPIO4",4,PinClass::Recommended,"Digitaler Ein-/Ausgang. ADC2_CH0, TOUCH0. Interne Pull-up/Pull-down-Widerstände vorhanden. Als Relais-Steuerausgang freigegeben.","Digital input/output. ADC2_CH0, TOUCH0. Internal pull-up/pull-down resistors available. Enabled as a relay-control output.","right",12},
    {"GPIO0",0,PinClass::Caution,"Boot-Strapping-Pin. LOW beim Reset startet den Download-/Flashmodus. Ein Relaismodul kann den Start verhindern. Nur verwenden, wenn der ESP mit angeschlossenem Relais sicher normal startet.","Boot strapping pin. LOW during reset selects download/flash mode. A relay module can prevent normal startup. Use only if the ESP reliably boots with the relay connected.","right",13},
    {"GPIO2",2,PinClass::Caution,"Boot-Strapping-Pin. Die externe Relaisbeschaltung kann beim Reset den Bootmodus beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","Boot strapping pin. External relay circuitry can affect boot mode at reset. Use only if startup and operation are reliable with the relay connected.","right",14},
    {"GPIO15",15,PinClass::Caution,"Boot-Strapping-Pin MTDO. Die externe Relaisbeschaltung kann beim Reset Boot-Ausgaben beeinflussen. Nur verwenden, wenn Start und Betrieb mit angeschlossenem Relais sicher funktionieren.","MTDO boot strapping pin. External relay circuitry can affect boot output at reset. Use only if startup and operation are reliable with the relay connected.","right",15},
    {"SD1/GPIO8",8,PinClass::Forbidden,"Mit dem SPI-Flash des WROOM-32 verbunden. Nicht als Anwender-GPIO verwenden.","Connected to the WROOM-32 SPI flash. Do not use as an application GPIO.","right",16},
    {"SD0/GPIO7",7,PinClass::Forbidden,"Mit dem SPI-Flash des WROOM-32 verbunden. Nicht als Anwender-GPIO verwenden.","Connected to the WROOM-32 SPI flash. Do not use as an application GPIO.","right",17},
    {"CLK/GPIO6",6,PinClass::Forbidden,"SPI-Flash-Takt des WROOM-32. Nicht als Anwender-GPIO verwenden.","WROOM-32 SPI-flash clock. Do not use as an application GPIO.","right",18},
  };
  return b;
}

const std::vector<BoardDefinition>& BoardProfiles::all() {
  static std::vector<BoardDefinition> defs = {make30(), make38()};
  return defs;
}

const BoardDefinition* BoardProfiles::find(const String& id) {
  for (const auto& b: all()) if (b.id==id) return &b;
  return nullptr;
}

bool BoardProfiles::relayAllowed(const String& profileId, int gpio, String& reason,const String& language) {
  const BoardDefinition* b=find(profileId);
  if (!b) { reason=language=="en"?"Unknown board profile":"Unbekanntes Platinenprofil"; return false; }
  for (const auto& p:b->pins) {
    if (p.gpio!=gpio) continue;
    reason=noteFor(p,language);
    return p.pinClass==PinClass::Recommended || p.pinClass==PinClass::Caution;
  }
  reason=language=="en"?"GPIO is not present as a relay output in the selected board profile":"GPIO ist im gewählten Platinenprofil nicht als Relaisausgang vorhanden";
  return false;
}

bool BoardProfiles::inputAllowed(const String& profileId, int gpio, String& reason,const String& language) {
  const BoardDefinition* b=find(profileId);
  if (!b) { reason=language=="en"?"Unknown board profile":"Unbekanntes Platinenprofil"; return false; }
  for (const auto& p:b->pins) {
    if (p.gpio!=gpio) continue;
    reason=noteFor(p,language);
    return p.pinClass==PinClass::Recommended || p.pinClass==PinClass::InputOnly;
  }
  reason=language=="en"?"GPIO is not enabled as an input in the selected board profile":"GPIO ist im gewählten Platinenprofil nicht als Eingang freigegeben";
  return false;
}


static const CustomBoardPinConfig* customPin(const CustomBoardConfig& board,int gpio){
  for(const auto& p:board.pins)if(p.kind=="gpio"&&p.gpio==gpio)return &p;
  return nullptr;
}

bool BoardProfiles::profileExists(const LocalConfig& config){
  if(config.boardProfile=="custom-user")return !config.customBoard.name.isEmpty()&&!config.customBoard.pins.empty();
  return find(config.boardProfile)!=nullptr;
}

bool BoardProfiles::relayAllowed(const LocalConfig& config,int gpio,String& reason){
  if(config.boardProfile!="custom-user")return relayAllowed(config.boardProfile,gpio,reason,config.language);
  const auto* p=customPin(config.customBoard,gpio);
  if(!p){reason=config.language=="en"?"GPIO is not present in the custom board profile":"GPIO ist im benutzerdefinierten Platinenprofil nicht vorhanden";return false;}
  if(p->reserved){reason=config.language=="en"?"Pin is marked as reserved by the user":"Pin ist vom Benutzer als reserviert markiert";return false;}
  if(!p->digitalOutput){reason=config.language=="en"?"Pin is not marked as a digital output":"Pin ist nicht als digitaler Ausgang freigegeben";return false;}
  if(gpio<0||gpio>63||!GPIO_IS_VALID_OUTPUT_GPIO(gpio)){reason=config.language=="en"?"This GPIO cannot physically work as an output on the compiled ESP target":"Dieser GPIO kann auf dem kompilierten ESP-Ziel technisch nicht als Ausgang arbeiten";return false;}
  reason=p->note;
  if(p->strapping){String w=config.language=="en"?"Boot/strapping pin - external circuitry can affect startup":"Boot-/Strapping-Pin – externe Beschaltung kann den Start beeinflussen";reason=reason.isEmpty()?w:w+" · "+reason;}
  return true;
}

bool BoardProfiles::diagnosticOutputAllowed(const LocalConfig& config,int gpio,String& reason){
  // Diagnostic probes may cover boot-strapping and UART pins so an unknown
  // relay-board channel can be identified. Flash pins and input-only pins stay
  // excluded even though some ESP targets report them as GPIO-capable.
  if(gpio<0||gpio>63||!GPIO_IS_VALID_OUTPUT_GPIO(gpio)||(gpio>=6&&gpio<=11)){
    reason=config.language=="en"?"GPIO is not a usable exposed output (input-only or reserved for module flash)":"GPIO ist kein nutzbarer herausgeführter Ausgang (nur Eingang oder für Modul-Flash reserviert)";
    return false;
  }
  if(config.boardProfile=="custom-user"){
    const auto* p=customPin(config.customBoard,gpio);
    if(!p||p->reserved||!p->digitalOutput){reason=config.language=="en"?"GPIO is not enabled as an output in the board profile":"GPIO ist im Platinenprofil nicht als Ausgang freigegeben";return false;}
    reason=p->note;
  }else{
    const auto* b=find(config.boardProfile);
    if(!b){reason=config.language=="en"?"Unknown board profile":"Unbekanntes Platinenprofil";return false;}
    bool found=false;
    for(const auto& p:b->pins)if(p.gpio==gpio){found=true;reason=noteFor(p,config.language);break;}
    if(!found){reason=config.language=="en"?"GPIO is not present on the selected board profile":"GPIO ist im gewählten Platinenprofil nicht vorhanden";return false;}
  }
  if(gpio==0||gpio==1||gpio==2||gpio==3||gpio==5||gpio==12||gpio==15){
    String warning=config.language=="en"?"Caution: boot-strapping/UART pin; test only after startup and expect boot/serial side effects":"Achtung: Boot-Strapping-/UART-Pin; nur im laufenden Betrieb testen, mögliche Boot-/Seriell-Nebenwirkungen beachten";
    reason=reason.isEmpty()?warning:warning+" · "+reason;
  }
  return true;
}

bool BoardProfiles::inputAllowed(const LocalConfig& config,int gpio,String& reason){
  if(config.boardProfile!="custom-user")return inputAllowed(config.boardProfile,gpio,reason,config.language);
  const auto* p=customPin(config.customBoard,gpio);
  if(!p){reason=config.language=="en"?"GPIO is not present in the custom board profile":"GPIO ist im benutzerdefinierten Platinenprofil nicht vorhanden";return false;}
  if(p->reserved){reason=config.language=="en"?"Pin is marked as reserved by the user":"Pin ist vom Benutzer als reserviert markiert";return false;}
  if(!p->digitalInput){reason=config.language=="en"?"Pin is not marked as a digital input":"Pin ist nicht als digitaler Eingang freigegeben";return false;}
  if(gpio<0||gpio>63||!GPIO_IS_VALID_GPIO(gpio)){reason=config.language=="en"?"This GPIO does not exist on the compiled ESP target":"Dieser GPIO existiert auf dem kompilierten ESP-Ziel nicht";return false;}
  reason=p->note;
  if(p->strapping){String w=config.language=="en"?"Boot/strapping pin - external circuitry can affect startup":"Boot-/Strapping-Pin – externe Beschaltung kann den Start beeinflussen";reason=reason.isEmpty()?w:w+" · "+reason;}
  return true;
}
