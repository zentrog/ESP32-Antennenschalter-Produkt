#include "Storage.h"
#include "BoardProfiles.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <set>
#include <esp_system.h>
#include <driver/gpio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>


static String littleFsVfsPath(const char* logical){return String("/littlefs")+String(logical);}
static bool quietFsExists(const char* logical){struct stat st{};String p=littleFsVfsPath(logical);return ::stat(p.c_str(),&st)==0;}
static bool quietFsRemove(const char* logical){if(!quietFsExists(logical))return true;String p=littleFsVfsPath(logical);return ::unlink(p.c_str())==0;}
static bool quietFsRename(const char* from,const char* to){String a=littleFsVfsPath(from),b=littleFsVfsPath(to);return ::rename(a.c_str(),b.c_str())==0;}

static String makeControllerId() {
  uint64_t m=ESP.getEfuseMac();
  char b[24]; snprintf(b,sizeof(b),"ESP32-%012llX",(unsigned long long)(m&0xFFFFFFFFFFFFULL));
  return String(b);
}



static String repairStoredText(String s){
  for(int pass=0;pass<4;++pass){
    String before=s;
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xA4","\xC3\xA4");
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xB6","\xC3\xB6");
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xBC","\xC3\xBC");
    s.replace("\xC3\x83\xC2\xA4","\xC3\xA4");
    s.replace("\xC3\x83\xC2\xB6","\xC3\xB6");
    s.replace("\xC3\x83\xC2\xBC","\xC3\xBC");
    s.replace("\xC3\x83\xC2\x84","\xC3\x84");
    s.replace("\xC3\x83\xC2\x96","\xC3\x96");
    s.replace("\xC3\x83\xC2\x9C","\xC3\x9C");
    s.replace("\xC3\x83\xC5\xB8","\xC3\x9F");
    s.replace("\xC3\x82\xC2\xB0","\xC2\xB0");
    s.replace("\xC3\x82\xC2\xA0"," ");
    s.replace("\xC3\xA2\xE2\x82\xAC\xE2\x80\x9C","\xE2\x80\x93");
    s.replace("\xC3\xA2\xE2\x82\xAC\xE2\x80\x9D","\xE2\x80\x94");
    if(s==before)break;
  }
  return s;
}

static bool isEn(const LocalConfig& c){ return c.language=="en"; }
static String L(const LocalConfig& c,const char* de,const char* en){ return isEn(c)?String(en):String(de); }
static void ensureLogicalDevicesFromLegacy(LocalConfig& c){
  if(!c.devices.empty())return;
  for(const auto&f:c.functions){
    if(!f.enabled||f.type!=FunctionType::Antenna)continue;
    LogicalDeviceConfig d;d.id="device-"+f.id;d.name=f.label;d.category="antenna";d.displayGroupId="antennas";d.functionIds.push_back(f.id);d.enabled=true;c.devices.push_back(d);
  }
}

bool Storage::begin() {
  // This filesystem contains the user's configuration. Never erase it just
  // because mounting failed; recovery must be explicit and must preserve data.
  if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) return false;
  state_.begin("state",false); errors_.begin("errors",false); wifi_.begin("wifi",false);
  return true;
}

void Storage::defaults(LocalConfig& c, SharedConfig& s) {
  c=LocalConfig{}; s=SharedConfig{};
  c.controllerId=makeControllerId();
  c.language="de";
  c.identity.callSign="";
  c.identity.postalCode="";
  c.identity.deviceName="Antennencontroller";
  c.identity.description="";
  c.identity.hostName="antenna-"+c.controllerId.substring(c.controllerId.length()-6);
  c.news.enabled=false;
  c.news.refreshMinutes=30;
  c.lightning.enabled=true;
  c.lightning.warningKm=50;
  c.lightning.dangerKm=25;
  c.lightning.boxKm=70;
  c.news.feeds.clear();
  c.federation.enabled=true;
  c.federation.systemId="";
  c.federation.systemName="";
  c.federation.udpPort=42142;
  c.federation.role="unassigned";
  c.federation.permanentMasterId="";
  c.federation.admissionMode="ask";
  c.federation.failoverPriority=100;
  c.relays.clear();c.functions.clear();c.devices.clear();
  s.systemTitle="";s.displayGroups.clear();s.layout.clear();s.routes.clear();
  s.revisionOrigin=c.controllerId;
  // Fresh installs and factory resets must not migrate example data back in.
  state_.putBool("mig120",true);state_.putBool("mig130",true);state_.putBool("mig150",true);
  state_.putBool("mig152",true);state_.putBool("mig153",true);state_.putBool("mig154",true);
  state_.putBool("mig155",true);state_.putBool("mig162",true);state_.putBool("mig173",true);
}


static void customBoardTo(JsonObject o,const CustomBoardConfig& b){
  o["name"]=b.name;o["note"]=b.note;auto a=o["pins"].to<JsonArray>();
  for(const auto& p:b.pins){auto q=a.add<JsonObject>();q["label"]=p.label;q["gpio"]=p.gpio;q["kind"]=p.kind;q["digitalInput"]=p.digitalInput;q["digitalOutput"]=p.digitalOutput;q["analogInput"]=p.analogInput;q["adcUnit"]=p.adcUnit;q["pullUp"]=p.pullUp;q["pullDown"]=p.pullDown;q["strapping"]=p.strapping;q["reserved"]=p.reserved;q["side"]=p.side;q["order"]=p.order;q["note"]=p.note;}
}
static void customBoardFrom(JsonVariantConst q,CustomBoardConfig& b){
  b=CustomBoardConfig{};if(q.isNull())return;b.name=repairStoredText(String((const char*)(q["name"]|"Benutzerdefiniertes Board")));b.note=repairStoredText(String((const char*)(q["note"]|"Pinbelegung und elektrische Eigenschaften wurden vom Benutzer eingetragen.")));b.pins.clear();
  for(JsonObjectConst o:q["pins"].as<JsonArrayConst>()){CustomBoardPinConfig p;p.label=repairStoredText(String((const char*)(o["label"]|"")));p.gpio=o["gpio"]|-1;p.kind=String((const char*)(o["kind"]|"gpio"));p.digitalInput=o["digitalInput"]|true;p.digitalOutput=o["digitalOutput"]|false;p.analogInput=o["analogInput"]|false;p.adcUnit=o["adcUnit"]|0;p.pullUp=o["pullUp"]|false;p.pullDown=o["pullDown"]|false;p.strapping=o["strapping"]|false;p.reserved=o["reserved"]|false;p.side=String((const char*)(o["side"]|"left"));p.order=o["order"]|0;p.note=repairStoredText(String((const char*)(o["note"]|"")));b.pins.push_back(p);}
}

static void identityTo(JsonObject o,const IdentityConfig& v){
  o["callSign"]=v.callSign;o["postalCode"]=v.postalCode;o["deviceName"]=v.deviceName;o["description"]=v.description;o["location"]=v.location;o["hostName"]=v.hostName;
}

static void stringVectorTo(JsonArray a,const std::vector<String>& values){for(const auto&v:values)a.add(v);}
static void stringVectorFrom(JsonVariantConst q,std::vector<String>& values){values.clear();for(JsonVariantConst v:q.as<JsonArrayConst>()){String x=String((const char*)(v|""));if(!x.isEmpty())values.push_back(x);}}
static void deviceTo(JsonObject o,const LogicalDeviceConfig& d){o["id"]=d.id;o["name"]=d.name;o["category"]=d.category;o["displayGroupId"]=d.displayGroupId;o["enabled"]=d.enabled;o["exclusive"]=d.exclusive;o["stormRelayId"]=d.stormRelayId;o["stormRelayOn"]=d.stormRelayOn;o["note"]=d.note;stringVectorTo(o["bands"].to<JsonArray>(),d.bands);stringVectorTo(o["functionIds"].to<JsonArray>(),d.functionIds);}
static void deviceFrom(JsonObjectConst q,LogicalDeviceConfig& d){d.id=String((const char*)(q["id"]|""));d.name=repairStoredText(String((const char*)(q["name"]|"")));d.category=String((const char*)(q["category"]|"other"));d.displayGroupId=String((const char*)(q["displayGroupId"]|""));d.enabled=q["enabled"]|true;d.exclusive=q["exclusive"]|false;d.stormRelayId=String((const char*)(q["stormRelayId"]|""));d.stormRelayOn=q["stormRelayOn"]|true;d.note=repairStoredText(String((const char*)(q["note"]|"")));stringVectorFrom(q["bands"],d.bands);stringVectorFrom(q["functionIds"],d.functionIds);}
static void displayGroupTo(JsonObject o,const DisplayGroupConfig& g){o["id"]=g.id;o["title"]=g.title;o["order"]=g.order;}
static void displayGroupFrom(JsonObjectConst q,DisplayGroupConfig& g){g.id=String((const char*)(q["id"]|""));g.title=repairStoredText(String((const char*)(q["title"]|"")));g.order=q["order"]|0;}
static void routeTo(JsonObject o,const RouteConfig& r){o["id"]=r.id;o["label"]=r.label;o["color"]=r.color;o["enabled"]=r.enabled;o["bandOverride"]=r.bandOverride;o["overrideNote"]=r.overrideNote;stringVectorTo(o["deviceKeys"].to<JsonArray>(),r.deviceKeys);}
static void routeFrom(JsonObjectConst q,RouteConfig& r){r.id=String((const char*)(q["id"]|""));r.label=repairStoredText(String((const char*)(q["label"]|"")));r.color=String((const char*)(q["color"]|"#3f8fd2"));r.enabled=q["enabled"]|true;r.bandOverride=q["bandOverride"]|false;r.overrideNote=repairStoredText(String((const char*)(q["overrideNote"]|"")));stringVectorFrom(q["deviceKeys"],r.deviceKeys);}

static void uiTo(JsonObject o,const UiDefaults& v){
  o["buttonWidth"]=v.buttonWidth;o["buttonHeight"]=v.buttonHeight;o["defaultCols"]=v.defaultCols;o["defaultRows"]=v.defaultRows;o["fontPx"]=v.fontPx;
  o["normalColor"]=v.normalColor;o["activeColor"]=v.activeColor;o["lockedColor"]=v.lockedColor;o["rememberedColor"]=v.rememberedColor;o["runningColor"]=v.runningColor;
}
static void uiFrom(JsonVariantConst q,UiDefaults& v){
  v.buttonWidth=q["buttonWidth"]|170;v.buttonHeight=q["buttonHeight"]|72;v.defaultCols=constrain((int)(q["defaultCols"]|3),1,12);v.defaultRows=constrain((int)(q["defaultRows"]|1),1,6);v.fontPx=q["fontPx"]|20;
  v.normalColor=String((const char*)(q["normalColor"]|"#275c91"));v.activeColor=String((const char*)(q["activeColor"]|"#259b55"));
  v.lockedColor=String((const char*)(q["lockedColor"]|"#555b65"));v.rememberedColor=String((const char*)(q["rememberedColor"]|"#b83232"));
  v.runningColor=String((const char*)(q["runningColor"]|"#d67d00"));
}

bool Storage::validate(const LocalConfig& c,String& err) const {
  if (c.language!="de" && c.language!="en") {err=L(c,"Oberflächensprache muss de oder en sein","Interface language must be de or en");return false;}
  if (c.controllerId.isEmpty()) {err=L(c,"Steuergerät-ID fehlt","Controller ID is missing");return false;}
  if(c.federation.role!="unassigned"&&c.federation.role!="master"&&c.federation.role!="follower"){err=L(c,"Ungültige Anlagenrolle","Invalid system role");return false;}
  if(c.federation.admissionMode!="off"&&c.federation.admissionMode!="ask"&&c.federation.admissionMode!="auto"){err=L(c,"Aufnahmemodus muss AUS, NACHFRAGEN oder AUTOMATISCH sein","Admission mode must be off, ask or auto");return false;}
  if(c.federation.role=="master"){if(c.federation.systemId.isEmpty()||c.federation.permanentMasterId!=c.controllerId){err=L(c,"Master-Rolle benötigt eine interne Anlagenkennung und dieses Gerät als permanenten Master","Master role requires an internal system identity and this controller as permanent master");return false;}}
  if(c.federation.role=="follower"){if(c.federation.systemId.isEmpty()||c.federation.permanentMasterId.isEmpty()||c.federation.permanentMasterId==c.controllerId){err=L(c,"Follower-Rolle benötigt eine Anlagenkennung und einen anderen permanenten Master","Follower role requires a system identity and another permanent master");return false;}}
  if (!BoardProfiles::profileExists(c)) {err=L(c,"Unbekanntes oder unvollständiges Platinenprofil","Unknown or incomplete board profile");return false;}
  if(c.boardProfile=="custom-user"){
    if(c.customBoard.name.isEmpty()){err=L(c,"Name des benutzerdefinierten Platinenprofils fehlt","Custom board profile name is missing");return false;}
    std::set<int> customGpios;
    for(const auto& p:c.customBoard.pins){
      if(p.label.isEmpty()){err=L(c,"Ein Pin im benutzerdefinierten Profil hat keine Beschriftung","A pin in the custom profile has no label");return false;}
      if(p.side!="left"&&p.side!="right"){err=L(c,"Pin-Seite muss links oder rechts sein","Pin side must be left or right");return false;}
      if(p.kind!="gpio"&&p.kind!="power"&&p.kind!="ground"&&p.kind!="control"&&p.kind!="nc"){err=L(c,"Ungültiger Pin-Typ im benutzerdefinierten Profil","Invalid pin type in custom profile");return false;}
      if(p.kind!="gpio")continue;
      if(p.gpio<0||p.gpio>63||!GPIO_IS_VALID_GPIO(p.gpio)){err=L(c,"Benutzerdefiniertes Profil enthält einen GPIO, der auf diesem ESP-Ziel technisch nicht existiert: ","Custom profile contains a GPIO that does not physically exist on this ESP target: ")+String(p.gpio);return false;}
      if(!customGpios.insert(p.gpio).second){err=L(c,"GPIO im benutzerdefinierten Profil doppelt vorhanden: ","GPIO occurs more than once in custom profile: ")+String(p.gpio);return false;}
      if(p.digitalOutput&&!GPIO_IS_VALID_OUTPUT_GPIO(p.gpio)){err=L(c,"GPIO kann auf diesem ESP-Ziel technisch kein Ausgang sein: ","GPIO cannot physically be an output on this ESP target: ")+String(p.gpio);return false;}
      if(p.adcUnit>2){err=L(c,"ADC-Angabe muss 0, 1 oder 2 sein","ADC unit must be 0, 1 or 2");return false;}
    }
  }
  if (c.identity.hostName.isEmpty()) {err=L(c,"Netzwerkname (Hostname) fehlt","Hostname is missing");return false;}
  if(!c.identity.postalCode.isEmpty()){if(c.identity.postalCode.length()!=5){err=L(c,"Postleitzahl muss leer oder genau fünfstellig sein","Postal code must be empty or exactly five digits");return false;}for(size_t k=0;k<c.identity.postalCode.length();++k)if(!isDigit(c.identity.postalCode[k])){err=L(c,"Postleitzahl darf nur Ziffern enthalten","Postal code may contain digits only");return false;}}
  if (c.time.enabled && (c.time.ntp1.isEmpty() && c.time.ntp2.isEmpty())) {err=L(c,"NTP aktiviert, aber kein Server gesetzt","NTP is enabled but no server is configured");return false;}
  if (c.news.language!="de" && c.news.language!="en") {err=L(c,"Meldungssprache muss de oder en sein","News language must be de or en");return false;}
  if (c.news.refreshMinutes<30 || c.news.refreshMinutes>1440) {err=L(c,"Meldungsintervall muss 30..1440 Minuten betragen","News interval must be 30..1440 minutes");return false;}
  if (c.news.maxItems<1 || c.news.maxItems>10) {err=L(c,"Maximale Meldungsanzahl muss 1..10 betragen","Maximum news items must be 1..10");return false;}
  if(c.lightning.dangerKm<1||c.lightning.dangerKm>300){err=L(c,"Gewitter-Gefahrenradius muss 1..300 km betragen","Lightning danger radius must be 1..300 km");return false;}
  if(c.lightning.warningKm<c.lightning.dangerKm||c.lightning.warningKm>500){err=L(c,"Gewitter-Warnradius muss mindestens dem Gefahrenradius entsprechen und darf höchstens 500 km betragen","Lightning warning radius must be at least the danger radius and at most 500 km");return false;}
  if(c.lightning.boxKm<c.lightning.warningKm||c.lightning.boxKm>600){err=L(c,"Gewitter-Empfangsradius muss mindestens dem Warnradius entsprechen und darf höchstens 600 km betragen","Lightning reception radius must be at least the warning radius and at most 600 km");return false;}
  if (c.txInterlock.enabled) {
    String why;
    if (!BoardProfiles::inputAllowed(c,c.txInterlock.gpio,why)) {err=L(c,"GPIO der TX-Sperre ist nicht erlaubt: ","TX interlock GPIO is not allowed: ")+why;return false;}
  }

  std::set<int> gpios; std::set<String> relayIds, functionIds, deviceIds;
  for (auto &r:c.relays) {
    if (!r.enabled) continue;
    if (r.id.isEmpty()) {err=L(c,"Relais-ID fehlt","Relay ID is missing");return false;}
    if (!relayIds.insert(r.id).second){err=L(c,"Doppelte Relais-ID ","Duplicate relay ID ")+r.id;return false;}
    String why;
    if (!BoardProfiles::relayAllowed(c,r.gpio,why)){err=L(c,"Relais ","Relay ")+r.name+": GPIO "+String(r.gpio)+L(c," nicht erlaubt ("," is not allowed (")+why+")";return false;}
    if(c.txInterlock.enabled && r.gpio==c.txInterlock.gpio){err="GPIO "+String(r.gpio)+L(c," ist zugleich Relaisausgang und Eingang der TX-Sperre"," is both a relay output and the TX interlock");return false;}
    if(!gpios.insert(r.gpio).second){err="GPIO "+String(r.gpio)+L(c," doppelt belegt"," is assigned more than once");return false;}
  }
  for(auto &f:c.functions){
    if(!f.enabled)continue;
    if(f.id.isEmpty()){err=L(c,"Funktions-ID fehlt","Function ID is missing");return false;}
    if(!functionIds.insert(f.id).second){err=L(c,"Doppelte Funktions-ID ","Duplicate function ID ")+f.id;return false;}
    if(f.type!=FunctionType::Storm && relayIds.find(f.relayId)==relayIds.end()){err=L(c,"Funktion ","Function ")+f.label+L(c," verweist auf unbekanntes Relais"," refers to an unknown relay");return false;}
    if(f.type==FunctionType::Antenna && f.group.isEmpty()){err=L(c,"Antennenfunktion ","Antenna function ")+f.label+L(c," benötigt eine Schaltgruppe"," requires a switching group");return false;}
    if(f.type==FunctionType::Timed && (f.durationMs<100 || f.durationMs>30000)){err=L(c,"Zeitaktion ","Timed action ")+f.label+L(c," muss 0,1..30 s sein"," must be 0.1..30 s");return false;}
  }
  for(auto &f:c.functions){
    if(!f.enabled || f.requiresFunctionId.isEmpty())continue;
    if(functionIds.find(f.requiresFunctionId)==functionIds.end()){err=L(c,"Abhängigkeit von ","Dependency of ")+f.label+L(c," ist ungültig"," is invalid");return false;}
  }
  const char* allowedCategories[]={"radio","antenna","pa","tuner","rotor","filter","preamp","transverter","switch","supply","measurement","other"};
  std::set<String> stormOnRelays,stormOffRelays;
  for(const auto&d:c.devices){
    if(!d.enabled)continue;
    if(d.id.isEmpty()||d.name.isEmpty()){err=L(c,"Anlagenteil benötigt ID und Namen","Logical device requires ID and name");return false;}
    if(!deviceIds.insert(d.id).second){err=L(c,"Doppelte Anlagenteil-ID ","Duplicate logical-device ID ")+d.id;return false;}
    bool catOk=false;for(const char*x:allowedCategories)if(d.category==x){catOk=true;break;}if(!catOk){err=L(c,"Unbekannte Anlagenteil-Kategorie: ","Unknown logical-device category: ")+d.category;return false;}
    std::set<String> seenFunctions;for(const auto&fid:d.functionIds){if(fid.isEmpty())continue;if(functionIds.find(fid)==functionIds.end()){err=L(c,"Anlagenteil ","Logical device ")+d.name+L(c," verweist auf unbekannte Schaltfunktion: "," references unknown switching function: ")+fid;return false;}if(!seenFunctions.insert(fid).second){err=L(c,"Schaltfunktion im Anlagenteil doppelt: ","Switching function occurs twice in logical device: ")+fid;return false;}}
    if(!d.stormRelayId.isEmpty()){if(relayIds.find(d.stormRelayId)==relayIds.end()){err=L(c,"Gewitterstellung von ","Storm position of ")+d.name+L(c," verweist auf unbekanntes Relais: "," references unknown relay: ")+d.stormRelayId;return false;}auto&same=d.stormRelayOn?stormOnRelays:stormOffRelays;auto&other=d.stormRelayOn?stormOffRelays:stormOnRelays;if(other.find(d.stormRelayId)!=other.end()){err=L(c,"Widersprüchliche Gewitterstellung für Relais ","Conflicting storm state for relay ")+d.stormRelayId;return false;}same.insert(d.stormRelayId);}
  }
  return true;
}

bool Storage::writeLocalFile(const char* path,const LocalConfig& c,String& err){
  JsonDocument d; d["schema"]=c.schema;d["language"]=c.language;d["revision"]=c.revision;d["controllerId"]=c.controllerId;d["boardProfile"]=c.boardProfile;customBoardTo(d["customBoard"].to<JsonObject>(),c.customBoard);
  identityTo(d["identity"].to<JsonObject>(),c.identity);
  auto t=d["time"].to<JsonObject>();t["enabled"]=c.time.enabled;t["tz"]=c.time.tz;t["ntp1"]=c.time.ntp1;t["ntp2"]=c.time.ntp2;t["showLocal"]=c.time.showLocal;t["showUtc"]=c.time.showUtc;t["showDate"]=c.time.showDate;
  auto m=d["mqtt"].to<JsonObject>();m["enabled"]=c.mqtt.enabled;m["host"]=c.mqtt.host;m["port"]=c.mqtt.port;m["user"]=c.mqtt.user;m["password"]=c.mqtt.password;m["baseTopic"]=c.mqtt.baseTopic;
  auto n=d["news"].to<JsonObject>();n["enabled"]=c.news.enabled;n["language"]=c.news.language;n["refreshMinutes"]=c.news.refreshMinutes;n["maxItems"]=c.news.maxItems;
  auto nf=n["feeds"].to<JsonArray>();for(auto &x:c.news.feeds){auto o=nf.add<JsonObject>();o["id"]=x.id;o["name"]=x.name;o["language"]=x.language;o["url"]=x.url;o["enabled"]=x.enabled;}
  auto li=d["lightning"].to<JsonObject>();li["enabled"]=c.lightning.enabled;li["warningKm"]=c.lightning.warningKm;li["dangerKm"]=c.lightning.dangerKm;li["boxKm"]=c.lightning.boxKm;
  auto f=d["federation"].to<JsonObject>();f["enabled"]=c.federation.enabled;f["systemId"]=c.federation.systemId;f["systemName"]=c.federation.systemName;f["udpPort"]=c.federation.udpPort;f["role"]=c.federation.role;f["permanentMasterId"]=c.federation.permanentMasterId;f["admissionMode"]=c.federation.admissionMode;f["failoverPriority"]=c.federation.failoverPriority;
  auto sec=d["security"].to<JsonObject>();sec["adminAuthEnabled"]=c.security.adminAuthEnabled;sec["adminUser"]=c.security.adminUser;sec["adminPassword"]=c.security.adminPassword;
  auto tx=d["txInterlock"].to<JsonObject>();tx["enabled"]=c.txInterlock.enabled;tx["gpio"]=c.txInterlock.gpio;tx["activeHigh"]=c.txInterlock.activeHigh;
  auto fe=d["features"].to<JsonObject>();fe["externalApi"]=c.features.externalApi;fe["ota"]=c.features.ota;
  uiTo(d["ui"].to<JsonObject>(),c.ui);
  auto rs=d["relays"].to<JsonArray>();for(auto&r:c.relays){auto o=rs.add<JsonObject>();o["id"]=r.id;o["name"]=r.name;o["gpio"]=r.gpio;o["activeLow"]=r.activeLow;o["enabled"]=r.enabled;}
  auto fs=d["functions"].to<JsonArray>();for(auto&x:c.functions){auto o=fs.add<JsonObject>();o["id"]=x.id;o["label"]=x.label;o["type"]=x.type==FunctionType::Timed?"timed":(x.type==FunctionType::Storm?"storm":"antenna");o["relayId"]=x.relayId;o["enabled"]=x.enabled;o["visible"]=x.visible;o["durationMs"]=x.durationMs;o["requiresFunctionId"]=x.requiresFunctionId;o["group"]=x.group;o["stateToken"]=x.stateToken;}
  auto ds=d["devices"].to<JsonArray>();for(const auto&x:c.devices)deviceTo(ds.add<JsonObject>(),x);
  File file=LittleFS.open(path,"w");if(!file){err="Datei kann nicht geschrieben werden";return false;}size_t z=serializeJson(d,file);file.flush();file.close();if(!z){err="JSON schreiben fehlgeschlagen";return false;}return true;
}

bool Storage::readLocalFile(const char* path,LocalConfig& c,String& err){
  File f=LittleFS.open(path,"r");if(!f){err="Datei fehlt";return false;}JsonDocument d;auto e=deserializeJson(d,f);f.close();if(e){err=String("JSON: ")+e.c_str();return false;}
  LocalConfig x;x.schema=d["schema"]|8;x.language=String((const char*)(d["language"]|"de"));x.revision=d["revision"]|1;x.controllerId=String((const char*)(d["controllerId"]|""));x.boardProfile=String((const char*)(d["boardProfile"]|"devkit-v1-30"));customBoardFrom(d["customBoard"],x.customBoard);
  JsonObject i=d["identity"].as<JsonObject>();x.identity.callSign=repairStoredText(String((const char*)(i["callSign"]|"")));x.identity.postalCode=String((const char*)(i["postalCode"]|""));x.identity.deviceName=repairStoredText(String((const char*)(i["deviceName"]|"Antennencontroller")));x.identity.description=repairStoredText(String((const char*)(i["description"]|"")));x.identity.location=repairStoredText(String((const char*)(i["location"]|"")));x.identity.hostName=String((const char*)(i["hostName"]|"antenna-controller"));
  JsonObject t=d["time"].as<JsonObject>();x.time.enabled=t["enabled"]|true;x.time.tz=String((const char*)(t["tz"]|"CET-1CEST,M3.5.0,M10.5.0/3"));x.time.ntp1=String((const char*)(t["ntp1"]|"pool.ntp.org"));x.time.ntp2=String((const char*)(t["ntp2"]|"time.cloudflare.com"));x.time.showLocal=t["showLocal"]|true;x.time.showUtc=t["showUtc"]|true;x.time.showDate=t["showDate"]|true;
  JsonObject m=d["mqtt"].as<JsonObject>();x.mqtt.enabled=m["enabled"]|false;x.mqtt.host=String((const char*)(m["host"]|""));x.mqtt.port=m["port"]|1883;x.mqtt.user=String((const char*)(m["user"]|""));x.mqtt.password=String((const char*)(m["password"]|""));x.mqtt.baseTopic=String((const char*)(m["baseTopic"]|"antenna"));
  JsonObject n=d["news"].as<JsonObject>();x.news.enabled=n["enabled"]|false;x.news.language=String((const char*)(n["language"]|"de"));x.news.refreshMinutes=n["refreshMinutes"]|30;x.news.maxItems=n["maxItems"]|5;
  for(JsonObject q:n["feeds"].as<JsonArray>()){NewsFeed z;z.id=String((const char*)(q["id"]|""));z.name=repairStoredText(String((const char*)(q["name"]|"")));z.language=String((const char*)(q["language"]|"de"));z.url=String((const char*)(q["url"]|""));z.enabled=q["enabled"]|true;x.news.feeds.push_back(z);}
  JsonObject li=d["lightning"].as<JsonObject>();x.lightning.enabled=li["enabled"]|true;x.lightning.warningKm=li["warningKm"]|50;x.lightning.dangerKm=li["dangerKm"]|25;x.lightning.boxKm=li["boxKm"]|70;
  JsonObject g=d["federation"].as<JsonObject>();x.federation.enabled=g["enabled"]|true;x.federation.systemId=String((const char*)(g["systemId"]|""));x.federation.systemName=repairStoredText(String((const char*)(g["systemName"]|"Antennenanlage")));x.federation.udpPort=g["udpPort"]|42142;x.federation.role=String((const char*)(g["role"]|"unassigned"));x.federation.permanentMasterId=String((const char*)(g["permanentMasterId"]|""));x.federation.admissionMode=String((const char*)(g["admissionMode"]|"ask"));x.federation.failoverPriority=g["failoverPriority"]|100;
  JsonObject s=d["security"].as<JsonObject>();x.security.adminAuthEnabled=s["adminAuthEnabled"]|false;x.security.adminUser=String((const char*)(s["adminUser"]|"admin"));x.security.adminPassword=String((const char*)(s["adminPassword"]|""));
  JsonObject tx=d["txInterlock"].as<JsonObject>();x.txInterlock.enabled=tx["enabled"]|false;x.txInterlock.gpio=tx["gpio"]|34;x.txInterlock.activeHigh=tx["activeHigh"]|true;
  JsonObject fe=d["features"].as<JsonObject>();x.features.externalApi=fe["externalApi"]|true;x.features.ota=fe["ota"]|true;uiFrom(d["ui"],x.ui);
  for(JsonObject q:d["relays"].as<JsonArray>()){RelayConfig z;z.id=String((const char*)(q["id"]|""));z.name=repairStoredText(String((const char*)(q["name"]|"")));z.gpio=q["gpio"]|-1;z.activeLow=q["activeLow"]|true;z.enabled=q["enabled"]|false;x.relays.push_back(z);}
  for(JsonObject q:d["functions"].as<JsonArray>()){FunctionConfig z;z.id=String((const char*)(q["id"]|""));z.label=repairStoredText(String((const char*)(q["label"]|"")));String ty=String((const char*)(q["type"]|"antenna"));z.type=ty=="timed"?FunctionType::Timed:(ty=="storm"?FunctionType::Storm:FunctionType::Antenna);z.relayId=String((const char*)(q["relayId"]|""));z.enabled=q["enabled"]|true;z.visible=q["visible"]|true;z.durationMs=q["durationMs"]|7500;z.requiresFunctionId=String((const char*)(q["requiresFunctionId"]|""));z.group=String((const char*)(q["group"]|"ANT"));z.stateToken=String((const char*)(q["stateToken"]|""));x.functions.push_back(z);}
  for(JsonObjectConst q:d["devices"].as<JsonArrayConst>()){LogicalDeviceConfig z;deviceFrom(q,z);x.devices.push_back(z);}
  if(!validate(x,err))return false;c=x;return true;
}

bool Storage::saveLocal(LocalConfig& c,String& err){
  if(!validate(c,err))return false;c.revision++;
  if(!writeLocalFile("/local.tmp",c,err))return false;
  LocalConfig check;String e;if(!readLocalFile("/local.tmp",check,e)){quietFsRemove("/local.tmp");err="Prüfung der neuen Konfiguration fehlgeschlagen: "+e;return false;}
  if(!quietFsRemove("/local.bak")){err="Alte Konfigurationssicherung konnte nicht entfernt werden";return false;}
  if(quietFsExists("/local.json")&&!quietFsRename("/local.json","/local.bak")){err="Aktuelle Konfiguration konnte nicht gesichert werden";return false;}
  if(!quietFsRename("/local.tmp","/local.json")){if(quietFsExists("/local.bak"))quietFsRename("/local.bak","/local.json");err="Aktivierung fehlgeschlagen";return false;}return true;
}
bool Storage::loadLocal(LocalConfig& c,String& err){
  if(readLocalFile("/local.json",c,err)){
    bool migrated=false;
    if(c.schema<6){ensureLogicalDevicesFromLegacy(c);c.schema=6;migrated=true;}
    if(c.schema<7){c.federation.systemId="";c.federation.systemName="Antennenanlage";c.federation.role="unassigned";c.federation.permanentMasterId="";c.federation.admissionMode="ask";c.federation.failoverPriority=100;c.schema=7;migrated=true;}
    if(c.schema<8){for(auto&d:c.devices)d.exclusive=false;c.schema=8;migrated=true;}
    if(c.schema<9){for(auto&d:c.devices){d.stormRelayId="";d.stormRelayOn=true;}c.schema=9;migrated=true;}
    if(c.schema<10){
      // Alte globale ANT-Gruppe war zu grob: jedes logisch unabhängige Anlagenteil
      // bekommt zunächst eine eigene Umschaltgruppe. Gemeinsame Gruppen können in
      // der Oberfläche bewusst wieder gleich benannt werden.
      for(auto&d:c.devices){for(const auto&fid:d.functionIds){for(auto&f:c.functions){if(f.id==fid&&f.type==FunctionType::Antenna&&f.group=="ANT"){f.group="DEV_"+d.id;break;}}}}
      c.schema=10;migrated=true;
    }
    if(c.schema<11){c.lightning.enabled=true;c.lightning.warningKm=50;c.lightning.dangerKm=25;c.lightning.boxKm=70;c.schema=11;migrated=true;}

    // One-time v1.3 migration: language, reliable additional feeds and a functional storm mode.
    if(!state_.getBool("mig130",false)){
      if(c.language!="de" && c.language!="en") c.language="de";
      auto ensureFeed=[&](const String&id,const String&name,const String&lang,const String&url,bool enabled){
        for(auto&f:c.news.feeds) if(f.id==id){f.name=name;f.language=lang;f.url=url;f.enabled=enabled;return;}
        c.news.feeds.push_back(NewsFeed(id,name,lang,url,enabled));
      };
      ensureFeed("nors","Nord-Ostsee-Rundspruch","de","https://nord-ostsee-rundspruch.de/feed/podcast/",true);
      ensureFeed("nors-main","Nord-Ostsee-Rundspruch Beiträge","de","https://nord-ostsee-rundspruch.de/feed/",true);
      ensureFeed("darc","DARC Aktuelles","de","https://www.darc.de/aktuelles/rss.xml",false);
      ensureFeed("ntv","n-tv Topmeldungen","de","https://www.n-tv.de/rss",false);
      ensureFeed("arrl","ARRL News","en","https://www.arrl.org/arrl.rss",true);
      ensureFeed("amsat","AMSAT News","en","https://www.amsat.org/feed/",true);
      bool hasStorm=false;for(auto&f:c.functions)if(f.type==FunctionType::Storm||f.id=="storm"){hasStorm=true;break;}
      if(!hasStorm){FunctionConfig storm;storm.id="storm";storm.label="GEWITTER";storm.type=FunctionType::Storm;storm.relayId="";storm.group="";storm.enabled=true;storm.visible=true;c.functions.push_back(storm);}
      migrated=true;
    }

    // One-time v1.5 migration: automatic LAN discovery, UTF-8 repair and fixed discovery port.
    if(!state_.getBool("mig150",false)){
      c.schema=8;
      c.federation.enabled=true;
      c.federation.udpPort=42142;
      c.identity.callSign=repairStoredText(c.identity.callSign);
      c.identity.deviceName=repairStoredText(c.identity.deviceName);
      c.identity.description=repairStoredText(c.identity.description);
      c.identity.location=repairStoredText(c.identity.location);
      for(auto&r:c.relays)r.name=repairStoredText(r.name);
      for(auto&f:c.functions)f.label=repairStoredText(f.label);
      for(auto&f:c.news.feeds)f.name=repairStoredText(f.name);
      auto ensure150=[&](const String&id,const String&name,const String&lang,const String&url,bool enabled){
        for(auto&f:c.news.feeds)if(f.id==id){f.name=name;f.language=lang;f.url=url;return;}
        c.news.feeds.push_back(NewsFeed(id,name,lang,url,enabled));
      };
      ensure150("nors","Nord-Ostsee-Rundspruch","de","https://nord-ostsee-rundspruch.de/feed/podcast/",true);
      ensure150("nors-main","Nord-Ostsee-Rundspruch Beiträge","de","https://nord-ostsee-rundspruch.de/feed/",true);
      ensure150("darc","DARC Aktuelles","de","https://www.darc.de/aktuelles/rss.xml",false);
      ensure150("ntv","n-tv Topmeldungen","de","https://www.n-tv.de/rss",false);
      ensure150("arrl","ARRL News","en","https://www.arrl.org/arrl.rss",true);
      ensure150("amsat","AMSAT News","en","https://www.amsat.org/feed/",true);
      migrated=true;
    }

    // One-time v1.5.2 migration: automatic controller discovery must be ON by default
    // also for installations that already completed the v1.5 migration.
    if(!state_.getBool("mig152",false)){
      c.federation.enabled=true;
      c.federation.udpPort=42142;
      c.identity.callSign=repairStoredText(c.identity.callSign);
      c.identity.deviceName=repairStoredText(c.identity.deviceName);
      c.identity.description=repairStoredText(c.identity.description);
      c.identity.location=repairStoredText(c.identity.location);
      for(auto&r:c.relays)r.name=repairStoredText(r.name);
      for(auto&f:c.functions)f.label=repairStoredText(f.label);
      for(auto&f:c.news.feeds)f.name=repairStoredText(f.name);
      migrated=true;
    }

    // One-time v1.5.3 migration: add a verified general-news feed disabled by default.
    if(!state_.getBool("mig153",false)){
      bool found=false;
      for(auto&f:c.news.feeds)if(f.id=="ntv"){
        f.name="n-tv Topmeldungen";f.language="de";f.url="https://www.n-tv.de/rss";
        found=true;break;
      }
      if(!found)c.news.feeds.push_back(NewsFeed("ntv","n-tv Topmeldungen","de","https://www.n-tv.de/rss",false));
      c.federation.enabled=true;
      c.federation.udpPort=42142;
      migrated=true;
    }

    // One-time v1.5.4 migration: discovery remains ON; n-tv feed remains available but disabled by default.
    if(!state_.getBool("mig154",false)){
      bool found=false;
      for(auto&f:c.news.feeds)if(f.id=="ntv"){f.name="n-tv Topmeldungen";f.language="de";f.url="https://www.n-tv.de/rss";found=true;break;}
      if(!found)c.news.feeds.push_back(NewsFeed("ntv","n-tv Topmeldungen","de","https://www.n-tv.de/rss",false));
      c.federation.enabled=true;c.federation.udpPort=42142;migrated=true;
    }


    // One-time v1.5.5 migration: controller discovery stays enabled and the
    // general-news source remains available. Layout behavior itself is stored
    // in shared.json and needs no destructive migration.
    if(!state_.getBool("mig155",false)){
      bool found=false;
      for(auto&f:c.news.feeds)if(f.id=="ntv"){f.name="n-tv Topmeldungen";f.language="de";f.url="https://www.n-tv.de/rss";found=true;break;}
      if(!found)c.news.feeds.push_back(NewsFeed("ntv","n-tv Topmeldungen","de","https://www.n-tv.de/rss",false));
      c.federation.enabled=true;c.federation.udpPort=42142;migrated=true;
    }
    // v1.6.2: neutral distribution defaults and a working German news baseline.
    if(!state_.getBool("mig162",false)){
      c.news.enabled=true;if(c.news.refreshMinutes<1)c.news.refreshMinutes=15;
      auto ensure162=[&](const String&id,const String&name,const String&lang,const String&url,bool enabled){for(auto&f:c.news.feeds)if(f.id==id){f.name=name;f.language=lang;f.url=url;f.enabled=enabled;return;}c.news.feeds.push_back(NewsFeed(id,name,lang,url,enabled));};
      ensure162("nors","Nord-Ostsee-Rundspruch","de","https://nord-ostsee-rundspruch.de/feed/podcast/",true);
      ensure162("dlrs","Deutschland-Rundspruch","de","https://nord-ostsee-rundspruch.de/category/deutschland-rundspruch/feed/",true);
      for(auto&f:c.news.feeds)if(f.id=="nors-main")f.enabled=false;
      migrated=true;
    }

    // v1.7.3: external news retrieval is no more frequent than every 30 minutes.
    if(!state_.getBool("mig173",false)){
      if(c.news.refreshMinutes<30)c.news.refreshMinutes=30;
      migrated=true;
    }

    if(migrated){
      String migrationError;
      if(!saveLocal(c,migrationError))err="Standardkonfiguration konnte nicht migriert werden: "+migrationError;
      else {state_.putBool("mig120",true);state_.putBool("mig130",true);state_.putBool("mig150",true);state_.putBool("mig152",true);state_.putBool("mig153",true);state_.putBool("mig154",true);state_.putBool("mig155",true);state_.putBool("mig162",true);state_.putBool("mig173",true);}
    }
    return true;
  }
  String bakErr;if(readLocalFile("/local.bak",c,bakErr)){writeLocalFile("/local.json",c,bakErr);err="Aktive Konfiguration ungültig; letzte gültige Konfiguration geladen";return true;}
  SharedConfig s;defaults(c,s);String x;return saveLocal(c,x);
}

bool Storage::writeSharedFile(const char* path,const SharedConfig& c,String& err){
  JsonDocument d;d["schema"]=c.schema;d["revision"]=c.revision;d["revisionOrigin"]=c.revisionOrigin;d["systemTitle"]=c.systemTitle;d["systemStormMode"]=c.systemStormMode;auto sec=d["security"].to<JsonObject>();sec["adminAuthEnabled"]=c.security.adminAuthEnabled;sec["adminUser"]=c.security.adminUser;sec["adminPassword"]=c.security.adminPassword;uiTo(d["ui"].to<JsonObject>(),c.ui);
  auto a=d["layout"].to<JsonArray>();for(auto&x:c.layout){auto o=a.add<JsonObject>();o["key"]=x.key;o["x"]=x.x;o["y"]=x.y;o["w"]=x.w;o["h"]=x.h;o["fontPx"]=x.fontPx;o["visible"]=x.visible;}
  auto dg=d["displayGroups"].to<JsonArray>();for(const auto&x:c.displayGroups)displayGroupTo(dg.add<JsonObject>(),x);
  auto rt=d["routes"].to<JsonArray>();for(const auto&x:c.routes)routeTo(rt.add<JsonObject>(),x);
  File f=LittleFS.open(path,"w");if(!f){err="Gemeinsame Anordnungsdatei ist nicht schreibbar";return false;}size_t z=serializeJson(d,f);f.flush();f.close();return z>0;
}
bool Storage::readSharedFile(const char* path,SharedConfig& c,String& err){
  File f=LittleFS.open(path,"r");if(!f){err="Gemeinsame Anordnungsdatei fehlt";return false;}JsonDocument d;auto e=deserializeJson(d,f);f.close();if(e){err=e.c_str();return false;}
  SharedConfig x;x.schema=d["schema"]|3;x.revision=d["revision"]|1;x.revisionOrigin=String((const char*)(d["revisionOrigin"]|""));x.systemTitle=repairStoredText(String((const char*)(d["systemTitle"]|"Antennensteuerung")));x.systemStormMode=d["systemStormMode"]|false;JsonObject sec=d["security"].as<JsonObject>();x.security.adminAuthEnabled=sec["adminAuthEnabled"]|false;x.security.adminUser=String((const char*)(sec["adminUser"]|"admin"));x.security.adminPassword=String((const char*)(sec["adminPassword"]|""));uiFrom(d["ui"],x.ui);
  for(JsonObject q:d["layout"].as<JsonArray>()){LayoutItem z;z.key=String((const char*)(q["key"]|""));z.x=q["x"]|0;z.y=q["y"]|0;z.w=q["w"]|3;z.h=q["h"]|1;z.fontPx=q["fontPx"]|20;z.visible=q["visible"]|true;x.layout.push_back(z);}
  for(JsonObjectConst q:d["displayGroups"].as<JsonArrayConst>()){DisplayGroupConfig z;displayGroupFrom(q,z);x.displayGroups.push_back(z);}
  for(JsonObjectConst q:d["routes"].as<JsonArrayConst>()){RouteConfig z;routeFrom(q,z);x.routes.push_back(z);}
 #if !defined(ANTCTRL_JUNGFRAU)
  if(x.displayGroups.empty())x.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};
 #endif
  c=x;return true;
}
bool Storage::saveShared(SharedConfig& c,String& err){
  if(c.revisionOrigin.isEmpty()){err="Ursprungskennung der gemeinsamen Anordnung fehlt";return false;}
  if(!writeSharedFile("/shared.tmp",c,err))return false;SharedConfig chk;String e;if(!readSharedFile("/shared.tmp",chk,e)){quietFsRemove("/shared.tmp");err=e;return false;}
  if(!quietFsRemove("/shared.bak")){err="Alte Anordnungssicherung konnte nicht entfernt werden";return false;}
  if(quietFsExists("/shared.json")&&!quietFsRename("/shared.json","/shared.bak")){err="Aktuelle Anordnung konnte nicht gesichert werden";return false;}
  if(!quietFsRename("/shared.tmp","/shared.json")){if(quietFsExists("/shared.bak"))quietFsRename("/shared.bak","/shared.json");err="Aktivierung der gemeinsamen Anordnung fehlgeschlagen";return false;}return true;
}
bool Storage::loadShared(SharedConfig& c,String& err){
  auto migrate=[this,&err](SharedConfig& x)->bool{
    if(x.schema>=5){
 #if !defined(ANTCTRL_JUNGFRAU)
      if(x.displayGroups.empty())x.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};
 #endif
      return true;
    }
    bool hadStorm=x.schema>=4;
    x.schema=5;
    if(!hadStorm)x.systemStormMode=false;
 #if !defined(ANTCTRL_JUNGFRAU)
    if(x.displayGroups.empty())x.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};
 #endif
    x.revision++;
    String e;
    if(!saveShared(x,e)){err="Umstellung der Anlagenstruktur auf Schema 5 fehlgeschlagen: "+e;return false;}
    return true;
  };
  if(readSharedFile("/shared.json",c,err))return migrate(c);
  String e;
  if(readSharedFile("/shared.bak",c,e)){if(!migrate(c))return false;return true;}
  LocalConfig l;defaults(l,c);return saveShared(c,e);
}

RuntimeState Storage::loadState(){
  RuntimeState s;
  String js=state_.isKey("sel")?state_.getString("sel",""):String();JsonDocument d;
  if(js.length() && !deserializeJson(d,js)){
    for(JsonPair kv:d.as<JsonObject>()){ActiveSelection a;a.group=String(kv.key().c_str());a.functionId=String((const char*)kv.value().as<const char*>());s.activeSelections.push_back(a);}
  }
  if(!s.activeSelections.empty())s.activeAntennaId=s.activeSelections[0].functionId;
  s.polarization=state_.isKey("pol")?state_.getString("pol",""):String();s.motorRunning=state_.getBool("motor",false);s.stormMode=state_.getBool("storm",false);return s;
}
void Storage::saveSelection(const String&group,const String&id){
  JsonDocument d;String js=state_.isKey("sel")?state_.getString("sel",""):String();if(js.length())deserializeJson(d,js);JsonObject o=d.is<JsonObject>()?d.as<JsonObject>():d.to<JsonObject>();
  if(id.isEmpty())o.remove(group);else o[group]=id;String out;serializeJson(d,out);state_.putString("sel",out);
}
void Storage::clearSelections(){state_.remove("sel");}
void Storage::motorStart(){state_.putBool("motor",true);}
void Storage::motorFinished(const String&t){state_.putString("pol",t);state_.putBool("motor",false);}
void Storage::motorUnknown(){state_.putString("pol","UNKNOWN");state_.putBool("motor",false);}
void Storage::saveStorm(bool active){state_.putBool("storm",active);}
void Storage::saveStormSnapshot(const RuntimeState& s){
  JsonDocument d;auto a=d["selections"].to<JsonArray>();for(const auto&x:s.activeSelections){auto o=a.add<JsonObject>();o["group"]=x.group;o["functionId"]=x.functionId;}d["polarization"]=s.polarization;String json;serializeJson(d,json);state_.putString("stormSnap",json);state_.putBool("stormSnapOk",true);
}
bool Storage::loadStormSnapshot(RuntimeState& s){
  if(!state_.getBool("stormSnapOk",false))return false;String json=state_.getString("stormSnap","");if(json.isEmpty())return false;JsonDocument d;if(deserializeJson(d,json))return false;s=RuntimeState{};for(JsonObjectConst q:d["selections"].as<JsonArrayConst>()){ActiveSelection x;x.group=String((const char*)(q["group"]|""));x.functionId=String((const char*)(q["functionId"]|""));if(!x.group.isEmpty()&&!x.functionId.isEmpty())s.activeSelections.push_back(x);}s.activeAntennaId=s.activeSelections.empty()?String():s.activeSelections[0].functionId;s.polarization=String((const char*)(d["polarization"]|""));return true;
}
void Storage::clearStormSnapshot(){state_.remove("stormSnap");state_.remove("stormSnapOk");}
bool Storage::loadWeatherLocation(const String& postal,float& lat,float& lon,String& place){
  if(postal.isEmpty()||state_.getString("geoPost","")!=postal)return false;
  lat=state_.getFloat("geoLat",0.0f);lon=state_.getFloat("geoLon",0.0f);place=state_.getString("geoPlace","");
  return lat>=-90.0f&&lat<=90.0f&&lon>=-180.0f&&lon<=180.0f&&(lat!=0.0f||lon!=0.0f)&&!place.isEmpty();
}
void Storage::saveWeatherLocation(const String& postal,float lat,float lon,const String& place){
  if(postal.isEmpty()||lat<-90.0f||lat>90.0f||lon<-180.0f||lon>180.0f||(lat==0.0f&&lon==0.0f))return;
  state_.putString("geoPost",postal);state_.putFloat("geoLat",lat);state_.putFloat("geoLon",lon);state_.putString("geoPlace",place);
}
uint16_t Storage::allocateFailoverPriority(){
  uint16_t current=state_.getUShort("foNext",100);if(current<100)current=100;uint32_t next=(uint32_t)current+10U;state_.putUShort("foNext",(uint16_t)(next>60000U?60000U:next));return current;
}
void Storage::observeFailoverPriority(uint16_t priority){
  if(priority<100)return;uint16_t current=state_.getUShort("foNext",100);if(current<100)current=100;if(priority>=current){uint32_t next=(uint32_t)priority+10U;state_.putUShort("foNext",(uint16_t)(next>60000U?60000U:next));}
}
void Storage::resetFailoverPrioritySequence(){state_.putUShort("foNext",100);}

void Storage::addError(const String&code,const String&text,uint32_t epoch){
  JsonDocument d;String s=errors_.isKey("ring")?errors_.getString("ring",""):String();if(s.length())deserializeJson(d,s);JsonArray a=d.is<JsonArray>()?d.as<JsonArray>():d.to<JsonArray>();while(a.size()>=10)a.remove(0);auto o=a.add<JsonObject>();o["ts"]=epoch;o["code"]=code;o["text"]=text;String out;serializeJson(d,out);errors_.putString("ring",out);
}
std::vector<ErrorEntry> Storage::errors(){std::vector<ErrorEntry> r;JsonDocument d;String s=errors_.isKey("ring")?errors_.getString("ring",""):String();if(!s.length()||deserializeJson(d,s))return r;for(JsonObject o:d.as<JsonArray>())r.push_back({o["ts"]|0U,String((const char*)(o["code"]|"")),String((const char*)(o["text"]|""))});return r;}
void Storage::clearErrors(){errors_.remove("ring");}


std::vector<WifiCredential> Storage::wifiNetworks(){
  std::vector<WifiCredential> out;
  String raw=wifi_.isKey("networks")?wifi_.getString("networks",""):String();
  if(raw.isEmpty()) return out;
  JsonDocument d;if(deserializeJson(d,raw))return out;
  for(JsonObject o:d.as<JsonArray>()){
    WifiCredential w;
    w.ssid=String((const char*)(o["ssid"]|""));
    w.password=String((const char*)(o["password"]|""));
    if(!w.ssid.isEmpty()) out.push_back(w);
    if(out.size()>=5) break;
  }
  return out;
}

static bool persistWifiList(Preferences& prefs,const std::vector<WifiCredential>& nets){
  JsonDocument d;JsonArray a=d.to<JsonArray>();
  for(const auto&w:nets){auto o=a.add<JsonObject>();o["ssid"]=w.ssid;o["password"]=w.password;}
  String raw;serializeJson(d,raw);return prefs.putString("networks",raw)>0;
}

void Storage::ensureDefaultWifi(){
  // Wi-Fi credentials are always supplied by the user or an authorized master.
  // Fresh installs and factory resets never contain development network data.
  if(!wifi_.getBool("initialized",false)){
    persistWifiList(wifi_,wifiNetworks());
    wifi_.putBool("initialized",true);
    wifi_.putBool("defaultsV12",true);
  }
}

bool Storage::addOrUpdateWifi(const String& ssid,const String& pass,bool highestPriority,String& err){
  String clean=ssid;clean.trim();
  if(clean.isEmpty()||clean.length()>32){err="SSID muss 1..32 Zeichen lang sein";return false;}
  if(pass.length()>63){err="WLAN-Passwort ist zu lang";return false;}
  auto nets=wifiNetworks();
  for(size_t i=0;i<nets.size();++i){
    if(nets[i].ssid==clean){
      WifiCredential x=nets[i];x.password=pass;nets.erase(nets.begin()+i);
      if(highestPriority)nets.insert(nets.begin(),x);else nets.push_back(x);
      if(!persistWifiList(wifi_,nets)){err="WLAN-Liste konnte nicht gespeichert werden";return false;}
      return true;
    }
  }
  if(nets.size()>=5){err="Maximal 5 WLANs können gespeichert werden";return false;}
  WifiCredential x{clean,pass};
  if(highestPriority)nets.insert(nets.begin(),x);else nets.push_back(x);
  if(!persistWifiList(wifi_,nets)){err="WLAN-Liste konnte nicht gespeichert werden";return false;}
  return true;
}

bool Storage::deleteWifi(size_t index,String&err){
  auto nets=wifiNetworks();if(index>=nets.size()){err="Ungültiger WLAN-Index";return false;}
  nets.erase(nets.begin()+index);
  if(!persistWifiList(wifi_,nets)){err="WLAN-Liste konnte nicht gespeichert werden";return false;}
  return true;
}

bool Storage::moveWifi(size_t index,int direction,String&err){
  auto nets=wifiNetworks();if(index>=nets.size()){err="Ungültiger WLAN-Index";return false;}
  int target=(int)index+direction;if(target<0||target>=(int)nets.size()){err="Priorität kann nicht weiter verschoben werden";return false;}
  std::swap(nets[index],nets[target]);
  if(!persistWifiList(wifi_,nets)){err="WLAN-Liste konnte nicht gespeichert werden";return false;}
  return true;
}

bool Storage::replaceWifiNetworks(const std::vector<WifiCredential>& incoming,String&err){
  if(incoming.size()>5){err="Maximal 5 WLANs können wiederhergestellt werden";return false;}
  std::vector<WifiCredential> nets;
  for(const auto&w:incoming){String ssid=w.ssid;ssid.trim();if(ssid.isEmpty()||ssid.length()>32){err="Ungültige SSID in der Sicherung";return false;}if(w.password.length()>63){err="WLAN-Passwort in der Sicherung ist zu lang";return false;}bool dup=false;for(const auto&x:nets)if(x.ssid==ssid){dup=true;break;}if(!dup)nets.push_back({ssid,w.password});}
  if(!persistWifiList(wifi_,nets)){err="WLAN-Liste konnte nicht wiederhergestellt werden";return false;}ensureDefaultWifi();return true;
}

void Storage::setForceSetup(){wifi_.putBool("forceSetup",true);}
bool Storage::consumeForceSetup(){bool v=wifi_.getBool("forceSetup",false);if(v)wifi_.putBool("forceSetup",false);return v;}

void Storage::factoryReset(){
  quietFsRemove("/local.json");quietFsRemove("/local.bak");quietFsRemove("/local.tmp");
  quietFsRemove("/shared.json");quietFsRemove("/shared.bak");quietFsRemove("/shared.tmp");
  state_.clear();errors_.clear();wifi_.clear();
}
