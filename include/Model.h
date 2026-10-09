#pragma once
#include <Arduino.h>
#include <vector>

enum class FunctionType : uint8_t { Antenna, Timed, Storm };
enum class PinClass : uint8_t { Recommended, Caution, Forbidden, InputOnly, Power, Ground, Control, NC };


struct CustomBoardPinConfig {
  String label;
  int gpio = -1;
  String kind = "gpio"; // gpio/power/ground/control/nc
  bool digitalInput = true;
  bool digitalOutput = false;
  bool analogInput = false;
  uint8_t adcUnit = 0; // 0=none/unknown, 1=ADC1, 2=ADC2
  bool pullUp = false;
  bool pullDown = false;
  bool strapping = false;
  bool reserved = false;
  String side = "left";
  int order = 0;
  String note;
};

struct CustomBoardConfig {
  String name = "Benutzerdefiniertes Board";
  String note = "Pinbelegung und elektrische Eigenschaften wurden vom Benutzer eingetragen.";
  std::vector<CustomBoardPinConfig> pins;
};


struct LogicalDeviceConfig {
  String id;
  String name;
  String category = "other"; // radio/antenna/pa/tuner/rotor/filter/preamp/transverter/switch/supply/measurement/other
  String displayGroupId;
  std::vector<String> bands;
  std::vector<String> functionIds; // local low-level switching functions used by this logical device
  bool enabled = true;
  bool exclusive = false; // only one simultaneously active route may use this logical device
  String stormRelayId;    // optional relay driven to a defined state while storm mode is active
  bool stormRelayOn = true;
  String note;
};

struct DisplayGroupConfig {
  String id;
  String title;
  int order = 0;

  DisplayGroupConfig() = default;
  DisplayGroupConfig(const String& id_, const String& title_, int order_)
      : id(id_), title(title_), order(order_) {}
};

struct RouteConfig {
  String id;
  String label;
  String color = "#3f8fd2";
  std::vector<String> deviceKeys; // ordered controllerId:logicalDeviceId path
  bool enabled = true;
  bool bandOverride = false;
  String overrideNote;
};

struct IdentityConfig {
  String callSign;
  String postalCode;
  String deviceName = "Antennencontroller";
  String description;
  String location;
  String hostName = "antenna-controller";
};

struct TimeConfig {
  bool enabled = true;
  String tz = "CET-1CEST,M3.5.0,M10.5.0/3";
  String ntp1 = "pool.ntp.org";
  String ntp2 = "time.cloudflare.com";
  bool showLocal = true;
  bool showUtc = true;
  bool showDate = true;
};

struct RelayConfig {
  String id;
  String name;
  int gpio = -1;
  bool activeLow = true;
  bool enabled = false;
};

struct FunctionConfig {
  String id;
  String label;
  FunctionType type = FunctionType::Antenna;
  String relayId;
  bool enabled = true;
  bool visible = true;
  uint32_t durationMs = 7500;
  String requiresFunctionId;
  String group = "ANT";
  String stateToken; // e.g. H/V for timed actions
};

struct UiDefaults {
  int buttonWidth = 170;   // backward compatibility with older backups
  int buttonHeight = 72;   // backward compatibility with older backups
  int defaultCols = 3;     // width of newly created antenna tiles in 12-column grid
  int defaultRows = 1;     // height of newly created antenna tiles in 6-row grid
  int fontPx = 20;
  String normalColor = "#275c91";
  String activeColor = "#259b55";
  String lockedColor = "#555b65";
  String rememberedColor = "#b83232";
  String runningColor = "#d67d00";
};

struct MqttConfig {
  bool enabled = false;
  String host;
  uint16_t port = 1883;
  String user;
  String password;
  String baseTopic = "antenna";
};

struct NewsFeed {
  String id;
  String name;
  String language; // de/en
  String url;
  bool enabled = true;

  NewsFeed() = default;
  NewsFeed(const String& id_, const String& name_, const String& language_, const String& url_, bool enabled_)
      : id(id_), name(name_), language(language_), url(url_), enabled(enabled_) {}
};

struct NewsConfig {
  bool enabled = true;
  String language = "de";
  uint16_t refreshMinutes = 30;
  uint8_t maxItems = 5;
  std::vector<NewsFeed> feeds;
};

struct LightningConfig {
  bool enabled = true;
  uint16_t warningKm = 50;
  uint16_t dangerKm = 25;
  uint16_t boxKm = 70;
};

struct FederationConfig {
  bool enabled = true;
  String systemId;                 // hidden internal installation identity
  String systemName = "Antennenanlage";
  uint16_t udpPort = 42142;
  String role = "unassigned";     // unassigned/master/follower
  String permanentMasterId;        // stable configured master, never rewritten by temporary failover
  String admissionMode = "ask";   // off/ask/auto
  uint16_t failoverPriority = 100; // lower value wins temporary replacement-master election
};

struct SecurityConfig {
  bool adminAuthEnabled = false;
  String adminUser = "admin";
  String adminPassword;
};

struct TxInterlockConfig {
  bool enabled = false;
  int gpio = 34;
  bool activeHigh = true;
};

struct FeatureConfig {
  bool externalApi = true;
  bool ota = true;
};

struct LocalConfig {
  uint32_t schema = 11;
  String language = "de";
  uint32_t revision = 1;
  String controllerId;
  String boardProfile = "devkit-v1-30";
  CustomBoardConfig customBoard;
  IdentityConfig identity;
  TimeConfig time;
  MqttConfig mqtt;
  NewsConfig news;
  LightningConfig lightning;
  FederationConfig federation;
  SecurityConfig security;
  TxInterlockConfig txInterlock;
  FeatureConfig features;
  UiDefaults ui;
  std::vector<RelayConfig> relays;
  std::vector<FunctionConfig> functions;
  std::vector<LogicalDeviceConfig> devices;
};

struct LayoutItem {
  String key; // controllerId:functionId
  int x = 0;
  int y = 0;
  int w = 3;
  int h = 1;
  int fontPx = 20;
  bool visible = true;
};

struct SharedConfig {
  uint32_t schema = 5;
  uint32_t revision = 1;
  String revisionOrigin;
  String systemTitle = "Antennensteuerung";
  bool systemStormMode = false;
  SecurityConfig security; // system-wide administrator protection; synchronized by the active master
  UiDefaults ui;
  std::vector<LayoutItem> layout;
  std::vector<DisplayGroupConfig> displayGroups;
  std::vector<RouteConfig> routes;
};

struct ActiveSelection { String group; String functionId; };

struct RuntimeState {
  std::vector<ActiveSelection> activeSelections;
  String activeAntennaId; // compatibility/default first selection
  String polarization; // H/V/UNKNOWN/""
  bool motorRunning = false;
  String motorFunctionId;
  bool stormMode = false;
};
