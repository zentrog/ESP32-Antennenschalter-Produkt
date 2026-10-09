#include "Services.h"
#include "Version.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <time.h>
#include <memory>
#include <stdlib.h>
#include <esp_system.h>

static SemaphoreHandle_t externalServiceMutex=nullptr;
static SemaphoreHandle_t ensureExternalServiceMutex(){if(!externalServiceMutex)externalServiceMutex=xSemaphoreCreateMutex();return externalServiceMutex;}

void TimeService::begin(const TimeConfig&c){if(c.enabled)configTzTime(c.tz.c_str(),c.ntp1.c_str(),c.ntp2.c_str());}
bool TimeService::synced()const{return time(nullptr)>1700000000;}
uint32_t TimeService::epoch()const{time_t t=time(nullptr);return t>0?(uint32_t)t:0;}
static String fmt(bool local){time_t t=time(nullptr);tm x{};if(local)localtime_r(&t,&x);else gmtime_r(&t,&x);char b[32];strftime(b,sizeof(b),"%d.%m.%Y %H:%M:%S",&x);return String(b);}
String TimeService::local()const{return fmt(true);}String TimeService::utc()const{return fmt(false);}

MqttService* MqttService::self_=nullptr;
void MqttService::begin(LocalConfig*c,RelayEngine*r){c_=c;r_=r;self_=this;mqtt_.setCallback(thunk);mqtt_.setSocketTimeout(2);}
void MqttService::configChanged(){if(mqtt_.connected())mqtt_.disconnect();lastTry_=0;lastPub_=0;}
String MqttService::base()const{return c_->mqtt.baseTopic+"/"+c_->controllerId;}
void MqttService::thunk(char*t,byte*p,unsigned int n){if(self_)self_->onMessage(t,p,n);}
void MqttService::onMessage(char*t,byte*p,unsigned int n){String topic=t,msg;for(unsigned i=0;i<n;i++)msg+=(char)p[i];if(topic==base()+"/cmd/function"){String e;r_->execute(msg,e);publishState();}}
void MqttService::publishState(){if(!c_||!c_->mqtt.enabled||!mqtt_.connected())return;auto&s=r_->state();mqtt_.publish((base()+"/status/online").c_str(),"1",true);mqtt_.publish((base()+"/status/antenna").c_str(),s.activeAntennaId.c_str(),true);JsonDocument sd;for(const auto&a:s.activeSelections)sd[a.group]=a.functionId;String sj;serializeJson(sd,sj);mqtt_.publish((base()+"/status/selections").c_str(),sj.c_str(),true);mqtt_.publish((base()+"/status/polarization").c_str(),s.polarization.c_str(),true);mqtt_.publish((base()+"/status/motor").c_str(),s.motorRunning?"1":"0",true);mqtt_.publish((base()+"/status/storm").c_str(),s.stormMode?"1":"0",true);}
void MqttService::loop(){
  if(!c_||!c_->mqtt.enabled||c_->mqtt.host.isEmpty()||WiFi.status()!=WL_CONNECTED)return;
  mqtt_.setServer(c_->mqtt.host.c_str(),c_->mqtt.port);
  if(!mqtt_.connected()&&millis()-lastTry_>10000){lastTry_=millis();String will=base()+"/status/online";bool ok=c_->mqtt.user.length()?mqtt_.connect(c_->controllerId.c_str(),c_->mqtt.user.c_str(),c_->mqtt.password.c_str(),will.c_str(),0,true,"0"):mqtt_.connect(c_->controllerId.c_str(),will.c_str(),0,true,"0");if(ok){mqtt_.subscribe((base()+"/cmd/function").c_str());publishState();}}
  if(mqtt_.connected()){mqtt_.loop();if(millis()-lastPub_>5000){lastPub_=millis();publishState();}}
}

static IPAddress subnetBroadcast(){
  IPAddress ip=WiFi.localIP(),mask=WiFi.subnetMask();
  return IPAddress(
    uint8_t(ip[0] | uint8_t(~mask[0])),
    uint8_t(ip[1] | uint8_t(~mask[1])),
    uint8_t(ip[2] | uint8_t(~mask[2])),
    uint8_t(ip[3] | uint8_t(~mask[3]))
  );
}

static String hiddenSystemId(){
  char out[33];
  uint32_t a=esp_random(),b=esp_random(),c=esp_random(),d=esp_random();
  snprintf(out,sizeof(out),"%08lX%08lX%08lX%08lX",(unsigned long)a,(unsigned long)b,(unsigned long)c,(unsigned long)d);
  return String(out);
}

static void parseSharedConfig(JsonDocument&d,SharedConfig&ns){
  int remoteSchema=d["schema"]|1;ns.schema=5;ns.systemStormMode=remoteSchema>=4?(d["systemStormMode"]|false):false;ns.revision=d["revision"]|1;ns.revisionOrigin=String((const char*)(d["revisionOrigin"]|""));ns.systemTitle=String((const char*)(d["systemTitle"]|"Antennensteuerung"));
  if(remoteSchema>=5){JsonObject sec=d["security"].as<JsonObject>();ns.security.adminAuthEnabled=sec["adminAuthEnabled"]|false;ns.security.adminUser=String((const char*)(sec["adminUser"]|"admin"));ns.security.adminPassword=String((const char*)(sec["adminPassword"]|""));}
  JsonObject u=d["ui"].as<JsonObject>();ns.ui.buttonWidth=u["buttonWidth"]|170;ns.ui.buttonHeight=u["buttonHeight"]|72;ns.ui.defaultCols=constrain((int)(u["defaultCols"]|3),1,12);ns.ui.defaultRows=constrain((int)(u["defaultRows"]|1),1,6);ns.ui.fontPx=u["fontPx"]|20;ns.ui.normalColor=String((const char*)(u["normalColor"]|"#275c91"));ns.ui.activeColor=String((const char*)(u["activeColor"]|"#259b55"));ns.ui.lockedColor=String((const char*)(u["lockedColor"]|"#555b65"));ns.ui.rememberedColor=String((const char*)(u["rememberedColor"]|"#b83232"));ns.ui.runningColor=String((const char*)(u["runningColor"]|"#d67d00"));
  if(remoteSchema>=2){for(JsonObject q:d["layout"].as<JsonArray>()){LayoutItem z;z.key=String((const char*)(q["key"]|""));z.x=constrain((int)(q["x"]|0),0,11);z.y=constrain((int)(q["y"]|0),0,5);z.w=constrain((int)(q["w"]|3),1,12);z.h=constrain((int)(q["h"]|1),1,6);if(z.x+z.w>12)z.x=12-z.w;if(z.y+z.h>6)z.y=6-z.h;z.fontPx=q["fontPx"]|20;z.visible=q["visible"]|true;ns.layout.push_back(z);}}
  if(remoteSchema>=3){
    for(JsonObject q:d["displayGroups"].as<JsonArray>()){DisplayGroupConfig g;g.id=String((const char*)(q["id"]|""));g.title=String((const char*)(q["title"]|""));g.order=q["order"]|0;if(!g.id.isEmpty()&&!g.title.isEmpty())ns.displayGroups.push_back(g);}
    for(JsonObject q:d["routes"].as<JsonArray>()){RouteConfig r;r.id=String((const char*)(q["id"]|""));r.label=String((const char*)(q["label"]|""));r.color=String((const char*)(q["color"]|"#3f8fd2"));r.enabled=q["enabled"]|true;r.bandOverride=q["bandOverride"]|false;r.overrideNote=String((const char*)(q["overrideNote"]|""));for(JsonVariant v:q["deviceKeys"].as<JsonArray>()){String key=String((const char*)(v|""));if(!key.isEmpty())r.deviceKeys.push_back(key);}if(!r.id.isEmpty())ns.routes.push_back(r);}
  }
  if(ns.displayGroups.empty())ns.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};
}

bool FederationService::isMaster()const{return c_&&c_->federation.role=="master"&&!c_->federation.systemId.isEmpty();}
bool FederationService::isFollower()const{return c_&&c_->federation.role=="follower"&&!c_->federation.systemId.isEmpty();}
bool FederationService::isUnassigned()const{return !c_||c_->federation.role=="unassigned"||c_->federation.systemId.isEmpty();}
bool FederationService::isCoordinator()const{return isMaster()?masterReady_:(isFollower()&&temporaryCoordinator_);}
bool FederationService::sameSystem(const PeerInfo&p)const{return c_&&!c_->federation.systemId.isEmpty()&&!p.systemId.isEmpty()&&p.systemId==c_->federation.systemId;}

bool FederationService::betterFallback(uint16_t aPriority,const String&aId,uint16_t bPriority,const String&bId)const{
  if(aPriority!=bPriority)return aPriority<bPriority;return aId<bId;
}

bool FederationService::networkWitness()const{
  if(WiFi.status()!=WL_CONNECTED)return false;
  IPAddress ip=WiFi.localIP(),gw=WiFi.gatewayIP();
  bool ipOk=ip[0]||ip[1]||ip[2]||ip[3],gwOk=gw[0]||gw[1]||gw[2]||gw[3];
  return ipOk&&gwOk;
}
bool FederationService::localBusy()const{return relay_&&relay_->state().motorRunning;}
bool FederationService::systemBusy(){
  if(localBusy())return true;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.busy)return true;
  return false;
}
bool FederationService::confirmSystemIdle(){
  if(localBusy())return false;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)){
    HTTPClient h;h.setTimeout(700);h.begin("http://"+p.ip+"/api/snapshot");int code=h.GET();String body=code==200?h.getString():String();h.end();if(code!=200)return false;JsonDocument d;if(deserializeJson(d,body))return false;if(d["motorRunning"]|false)return false;
  }
  return true;
}

bool FederationService::permanentMasterOnline()const{
  if(isMaster())return true;if(!isFollower()||!c_)return false;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.controllerId==c_->federation.permanentMasterId&&p.role=="master")return true;
  return false;
}
String FederationService::activeCoordinatorId()const{
  if(isCoordinator()&&c_)return c_->controllerId;
  if(!c_)return String();
  const PeerInfo*best=nullptr;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.coordinator){
    if(p.controllerId==c_->federation.permanentMasterId)return p.controllerId;
    if(!best||betterFallback(p.failoverPriority,p.controllerId,best->failoverPriority,best->controllerId))best=&p;
  }
  return best?best->controllerId:String();
}
String FederationService::systemHost()const{
  if(!c_||c_->federation.systemId.isEmpty())return String();
  String id=c_->federation.systemId;id.toLowerCase();
  String suffix=id.length()>=4?id.substring(id.length()-4):id;
  return String("antenne-")+suffix;
}
bool FederationService::masterOnline()const{return !activeCoordinatorId().isEmpty();}
String FederationService::masterIp()const{
  if(isCoordinator())return WiFi.localIP().toString();String id=activeCoordinatorId();if(id.isEmpty())return String();
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.controllerId==id)return p.ip;return String();
}
bool FederationService::authorizedCoordinator(const String&id)const{
  if(!c_||id.isEmpty())return false;
  if(id==c_->federation.permanentMasterId)return true;
  if(id==activeCoordinatorId())return true;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.coordinator&&p.controllerId==id)return true;
  return false;
}
String FederationService::electedFallbackId()const{
  if(!isFollower()||!c_)return String();String bestId=c_->controllerId;uint16_t bestPriority=c_->federation.failoverPriority;
  for(const auto&p:peers_)if(p.online&&sameSystem(p)&&p.role=="follower"&&p.permanentMasterId==c_->federation.permanentMasterId){
    if(betterFallback(p.failoverPriority,p.controllerId,bestPriority,bestId)){bestPriority=p.failoverPriority;bestId=p.controllerId;}
  }
  return bestId;
}

void FederationService::addSystemHeaders(HTTPClient&h)const{
  h.addHeader("X-Ant-Controller","ANTCTRL3");
  if(c_){h.addHeader("X-Ant-System",c_->federation.systemId);h.addHeader("X-Ant-From",c_->controllerId);}
}

void FederationService::begin(LocalConfig*c,SharedConfig*s,Storage*st,RelayEngine*r){
  c_=c;s_=s;store_=st;relay_=r;startedAt_=millis();lastHello_=lastSweep_=lastBootstrap_=lastAutoAdopt_=0;temporaryCoordinator_=false;masterReady_=false;reclaiming_=isMaster();masterMissingSince_=0;reclaimStableSince_=0;networkWasConnected_=WiFi.status()==WL_CONNECTED;
  if(c_&&c_->federation.enabled&&networkWasConnected_){udp_.begin(c_->federation.udpPort);sendHello();}
}
void FederationService::configChanged(){
  udp_.stop();lastHello_=lastSweep_=lastBootstrap_=lastAutoAdopt_=0;startedAt_=millis();temporaryCoordinator_=false;masterReady_=false;reclaiming_=isMaster();masterMissingSince_=0;reclaimStableSince_=0;networkWasConnected_=WiFi.status()==WL_CONNECTED;
  if(c_&&c_->federation.enabled&&networkWasConnected_){udp_.begin(c_->federation.udpPort);sendHello();}
}
std::vector<PeerInfo> FederationService::peers()const{return peers_;}

void FederationService::sendHello(){
  if(!c_||!c_->federation.enabled||WiFi.status()!=WL_CONNECTED)return;
  JsonDocument d;d["magic"]="ANTCTRL3";d["controllerId"]=c_->controllerId;d["name"]=c_->identity.deviceName;d["callSign"]=c_->identity.callSign;d["location"]=c_->identity.location;d["firmware"]=ANTCTRL_VERSION;d["boardProfile"]=c_->boardProfile;d["role"]=c_->federation.role;d["systemId"]=c_->federation.systemId;d["systemName"]=c_->federation.systemName;d["masterId"]=c_->federation.permanentMasterId;d["admission"]=c_->federation.admissionMode;d["priority"]=c_->federation.failoverPriority;d["coordinator"]=isCoordinator();d["busy"]=localBusy();d["rev"]=s_?s_->revision:0;d["origin"]=s_?s_->revisionOrigin:String();String x;serializeJson(d,x);
  IPAddress bc=subnetBroadcast();udp_.beginPacket(bc,c_->federation.udpPort);udp_.write((const uint8_t*)x.c_str(),x.length());udp_.endPacket();
}
void FederationService::upsert(const PeerInfo&p){for(auto&x:peers_)if(x.controllerId==p.controllerId){x=p;return;}peers_.push_back(p);}
void FederationService::receive(){
  int n=udp_.parsePacket();if(n<=0)return;String raw;while(udp_.available())raw+=(char)udp_.read();JsonDocument d;if(deserializeJson(d,raw))return;if(String((const char*)(d["magic"]|""))!="ANTCTRL3")return;
  String id=String((const char*)(d["controllerId"]|""));if(id.isEmpty()||!c_||id==c_->controllerId)return;
  PeerInfo p;p.controllerId=id;p.name=String((const char*)(d["name"]|""));p.callSign=String((const char*)(d["callSign"]|""));p.location=String((const char*)(d["location"]|""));p.firmware=String((const char*)(d["firmware"]|""));p.boardProfile=String((const char*)(d["boardProfile"]|""));p.ip=udp_.remoteIP().toString();p.systemId=String((const char*)(d["systemId"]|""));p.systemName=String((const char*)(d["systemName"]|""));p.role=String((const char*)(d["role"]|"unassigned"));p.permanentMasterId=String((const char*)(d["masterId"]|""));p.admissionMode=String((const char*)(d["admission"]|"off"));p.failoverPriority=d["priority"]|100;p.coordinator=d["coordinator"]|false;p.busy=d["busy"]|false;p.lastSeen=millis();p.online=true;p.sharedRevision=d["rev"]|0;p.revisionOrigin=String((const char*)(d["origin"]|""));upsert(p);
  if(isMaster()&&sameSystem(p)&&p.role=="follower"&&store_)store_->observeFailoverPriority(p.failoverPriority);
  for(auto&x:peers_)if(x.controllerId==id){if(sameSystem(x))synchronize(x);break;}
}

uint16_t FederationService::allocateFollowerPriority(){return store_?store_->allocateFailoverPriority():100;}

bool FederationService::promoteSelfToMaster(String&error){
  if(!c_||!store_){error="Interner Systemzustand fehlt";return false;}
  LocalConfig n=*c_;n.schema=11;n.federation.systemId=hiddenSystemId();n.federation.role="master";n.federation.permanentMasterId=n.controllerId;n.federation.failoverPriority=0;if(n.federation.systemName.isEmpty())n.federation.systemName="Antennenanlage";if(n.federation.admissionMode!="off"&&n.federation.admissionMode!="ask"&&n.federation.admissionMode!="auto")n.federation.admissionMode="ask";
  if(!store_->saveLocal(n,error))return false;*c_=n;store_->resetFailoverPrioritySequence();if(s_){s_->revision++;s_->revisionOrigin=c_->controllerId;String se;if(!store_->saveShared(*s_,se)){store_->addError("SYSTEM_SHARED",se);}}
  configChanged();return true;
}
bool FederationService::becomeOwnMaster(String&error){if(isMaster())return true;if(!isUnassigned()){error=c_&&c_->language=="en"?"Controller is already assigned":"Steuergerät ist bereits einer Anlage zugeordnet";return false;}return promoteSelfToMaster(error);}
bool FederationService::acceptAssignment(const String&systemId,const String&systemName,const String&masterId,uint16_t failoverPriority,String&error){
  if(!c_||!store_){error="Interner Systemzustand fehlt";return false;}if(!isUnassigned()){error=c_->language=="en"?"Controller is already assigned":"Steuergerät ist bereits einer Anlage zugeordnet";return false;}if(systemId.isEmpty()||masterId.isEmpty()||masterId==c_->controllerId){error=c_->language=="en"?"Invalid system assignment":"Ungültige Anlagenzuordnung";return false;}
  LocalConfig n=*c_;n.schema=11;n.federation.systemId=systemId;n.federation.systemName=systemName.isEmpty()?String("Antennenanlage"):systemName;n.federation.role="follower";n.federation.permanentMasterId=masterId;n.federation.admissionMode="off";n.federation.failoverPriority=failoverPriority<100?100:failoverPriority;
  if(!store_->saveLocal(n,error))return false;*c_=n;configChanged();return true;
}
bool FederationService::setAdmissionMode(const String&mode,String&error){
  if(!isMaster()){error=c_&&c_->language=="en"?"Only the permanent master can change admission":"Nur der permanente Master kann die Aufnahme steuern";return false;}if(mode!="off"&&mode!="ask"&&mode!="auto"){error=c_->language=="en"?"Invalid admission mode":"Ungültiger Aufnahmemodus";return false;}
  LocalConfig n=*c_;n.federation.admissionMode=mode;if(!store_->saveLocal(n,error))return false;*c_=n;sendHello();return true;
}

bool FederationService::sendAdopt(PeerInfo&p,bool ownMaster,String&error){
  if(!p.online||p.role!="unassigned"){error=c_&&c_->language=="en"?"New controller is no longer available":"Neues Steuergerät ist nicht mehr verfügbar";return false;}
  uint16_t priority=ownMaster?0:allocateFollowerPriority();HTTPClient h;h.setTimeout(1800);h.begin("http://"+p.ip+(ownMaster?"/api/federation/make-master":"/api/federation/adopt"));h.addHeader("X-Ant-Controller","ANTCTRL3-ONBOARD");h.addHeader("Content-Type","application/json");JsonDocument d;if(!ownMaster){d["systemId"]=c_->federation.systemId;d["systemName"]=c_->federation.systemName;d["masterId"]=c_->controllerId;d["failoverPriority"]=priority;}String body;serializeJson(d,body);int code=h.POST(body);String response=h.getString();h.end();if(code>=200&&code<300){p.role=ownMaster?"master":"follower";p.systemId=ownMaster?String("pending-foreign"):c_->federation.systemId;p.systemName=ownMaster?String("Neue Anlage"):c_->federation.systemName;p.permanentMasterId=ownMaster?p.controllerId:c_->controllerId;p.failoverPriority=priority;return true;}
  JsonDocument r;if(!deserializeJson(r,response))error=String((const char*)(r["error"]|""));if(error.isEmpty())error="HTTP "+String(code);return false;
}
bool FederationService::adoptPeer(const String&id,String&error){
  if(!isMaster()){error=c_&&c_->language=="en"?"This controller is not the master":"Dieses Steuergerät ist nicht der Master";return false;}if(c_->federation.admissionMode=="off"){error=c_->language=="en"?"Admission of new controllers is disabled":"Aufnahme neuer Steuergeräte ist ausgeschaltet";return false;}
  for(auto&p:peers_)if(p.controllerId==id)return sendAdopt(p,false,error);error=c_->language=="en"?"Controller not found":"Steuergerät nicht gefunden";return false;
}
bool FederationService::makePeerMaster(const String&id,String&error){
  if(!isMaster()){error=c_&&c_->language=="en"?"This controller is not the master":"Dieses Steuergerät ist nicht der Master";return false;}if(c_->federation.admissionMode=="off"){error=c_->language=="en"?"Admission of new controllers is disabled":"Aufnahme neuer Steuergeräte ist ausgeschaltet";return false;}
  for(auto&p:peers_)if(p.controllerId==id)return sendAdopt(p,true,error);error=c_->language=="en"?"Controller not found":"Steuergerät nicht gefunden";return false;
}

bool FederationService::pullSharedFrom(PeerInfo&p){
  if(!sameSystem(p)||!s_||!store_||!p.online)return false;HTTPClient h;h.setTimeout(1800);h.begin("http://"+p.ip+"/api/shared?internal=1");addSystemHeaders(h);int code=h.GET();String body=code==200?h.getString():String();h.end();if(code!=200)return false;JsonDocument d;if(deserializeJson(d,body))return false;SharedConfig ns;parseSharedConfig(d,ns);String e;if(!store_->saveShared(ns,e)){store_->addError("SHARED_PULL",e);return false;}*s_=ns;if(relay_)relay_->setStormMode(ns.systemStormMode);return true;
}
void FederationService::pushSharedToFollowers(){
  if(!isCoordinator()||!c_||!s_)return;
  for(auto&p:peers_)if(p.online&&sameSystem(p)&&p.role=="follower"&&p.controllerId!=c_->controllerId&&(p.sharedRevision!=s_->revision||p.revisionOrigin!=s_->revisionOrigin)){
    HTTPClient h;h.setTimeout(1800);h.begin("http://"+p.ip+"/api/federation/pull-shared");addSystemHeaders(h);h.addHeader("Content-Type","application/json");JsonDocument d;d["ip"]=WiFi.localIP().toString();String body;serializeJson(d,body);h.POST(body);h.end();
  }
}
void FederationService::synchronize(PeerInfo&p){
  if(!sameSystem(p)||!s_||!store_)return;
  if(isCoordinator()){
    if(p.role=="follower"&&(p.sharedRevision!=s_->revision||p.revisionOrigin!=s_->revisionOrigin))pushSharedToFollowers();
    return;
  }
  if(p.coordinator&&(p.sharedRevision!=s_->revision||p.revisionOrigin!=s_->revisionOrigin))pullSharedFrom(p);
}
void FederationService::notifySharedChanged(){if(!isCoordinator()||!c_->federation.enabled)return;sendHello();pushSharedToFollowers();}
bool FederationService::fetchRemoteSnapshot(const PeerInfo&p,String&json,uint16_t timeoutMs){if(!sameSystem(p))return false;HTTPClient h;h.setTimeout(timeoutMs);h.begin("http://"+p.ip+"/api/snapshot");int code=h.GET();if(code==200)json=h.getString();h.end();return code==200;}
bool FederationService::forwardExecute(const String&cid,const String&fid,String&response){
  if(!isCoordinator()){response=c_&&c_->language=="en"?"{\"ok\":false,\"error\":\"Only the active master may route remote commands\"}":"{\"ok\":false,\"error\":\"Nur der aktive Master darf Fernschaltungen verteilen\"}";return false;}
  for(auto&p:peers_)if(p.controllerId==cid&&p.online&&sameSystem(p)){HTTPClient h;h.setTimeout(1500);h.begin("http://"+p.ip+"/api/execute");addSystemHeaders(h);h.addHeader("Content-Type","application/json");JsonDocument d;d["id"]=fid;String b;serializeJson(d,b);int code=h.POST(b);response=h.getString();h.end();return code>=200&&code<300;}response=c_&&c_->language=="en"?"{\"ok\":false,\"error\":\"Controller offline/unknown\"}":"{\"ok\":false,\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}";return false;
}

void FederationService::updateFailover(){
  if(!c_||isUnassigned())return;
  uint32_t now=millis();
  if(isMaster()){
    temporaryCoordinator_=false;
    if(masterReady_){reclaiming_=false;return;}
    reclaiming_=true;
    PeerInfo*replacement=nullptr;
    for(auto&p:peers_)if(p.online&&sameSystem(p)&&p.role=="follower"&&p.coordinator){if(!replacement||betterFallback(p.failoverPriority,p.controllerId,replacement->failoverPriority,replacement->controllerId))replacement=&p;}
    if(now-startedAt_<9000){reclaimStableSince_=0;return;}
    if(replacement){
      if(s_&&(replacement->sharedRevision!=s_->revision||replacement->revisionOrigin!=s_->revisionOrigin)){if(!pullSharedFrom(*replacement)){reclaimStableSince_=0;return;}sendHello();}
      if(s_&&(replacement->sharedRevision!=s_->revision||replacement->revisionOrigin!=s_->revisionOrigin)){reclaimStableSince_=0;return;}
    }
    if(systemBusy()){reclaimStableSince_=0;return;}
    if(!reclaimStableSince_)reclaimStableSince_=now;
    if(now-reclaimStableSince_>=2500){if(!confirmSystemIdle()){reclaimStableSince_=0;return;}masterReady_=true;reclaiming_=false;reclaimStableSince_=0;sendHello();pushSharedToFollowers();}
    return;
  }

  if(!isFollower())return;
  bool permanentSeen=permanentMasterOnline();
  PeerInfo*permanent=nullptr;PeerInfo*otherCoordinator=nullptr;
  for(auto&p:peers_)if(p.online&&sameSystem(p)){
    if(p.controllerId==c_->federation.permanentMasterId&&p.role=="master")permanent=&p;
    if(p.coordinator&&p.controllerId!=c_->controllerId&&p.role=="follower"){
      if(!otherCoordinator||betterFallback(p.failoverPriority,p.controllerId,otherCoordinator->failoverPriority,otherCoordinator->controllerId))otherCoordinator=&p;
    }
  }
  if(permanentSeen){
    masterMissingSince_=0;
    if(temporaryCoordinator_&&permanent&&permanent->coordinator&&!localBusy()){temporaryCoordinator_=false;reclaimStableSince_=0;sendHello();}
    return;
  }
  if(!masterMissingSince_)masterMissingSince_=now;
  if(temporaryCoordinator_){
    if(otherCoordinator&&betterFallback(otherCoordinator->failoverPriority,otherCoordinator->controllerId,c_->federation.failoverPriority,c_->controllerId)&&!localBusy()){temporaryCoordinator_=false;sendHello();return;}
    if(!networkWitness()&&!localBusy()){temporaryCoordinator_=false;sendHello();return;}
    return;
  }
  if(otherCoordinator)return;
  if(now-masterMissingSince_<20000)return;
  if(!networkWitness())return;
  if(electedFallbackId()==c_->controllerId){
    PeerInfo*newest=nullptr;if(s_)for(auto&p:peers_)if(p.online&&sameSystem(p)&&p.role=="follower"&&p.sharedRevision>s_->revision){if(!newest||p.sharedRevision>newest->sharedRevision)newest=&p;}
    if(newest&&!pullSharedFrom(*newest))return;
    temporaryCoordinator_=true;reclaimStableSince_=0;sendHello();pushSharedToFollowers();
  }
}

void FederationService::bootstrap(){
  if(!c_||!store_||!c_->federation.enabled||WiFi.status()!=WL_CONNECTED)return;
  if(isUnassigned()){
    bool masterSeen=false;String smallest=c_->controllerId;
    for(const auto&p:peers_)if(p.online){if(p.role=="master"&&!p.systemId.isEmpty())masterSeen=true;if(p.role=="unassigned"&&p.controllerId<smallest)smallest=p.controllerId;}
    if(!masterSeen&&millis()-startedAt_>=8000&&smallest==c_->controllerId){String e;if(!promoteSelfToMaster(e))store_->addError("MASTER_BOOTSTRAP",e);}
    return;
  }
  if(isMaster()&&c_->federation.admissionMode=="auto"&&millis()-lastAutoAdopt_>=2500){
    lastAutoAdopt_=millis();size_t capable=1;for(const auto&p:peers_)if(p.online&&p.role=="master"&&!p.systemId.isEmpty()&&p.admissionMode!="off")capable++;
    if(capable==1){for(auto&p:peers_)if(p.online&&p.role=="unassigned"){String e;if(!sendAdopt(p,false,e))store_->addError("AUTO_ADOPT",e);break;}}
  }
}
void FederationService::loop(){
  if(!c_||!c_->federation.enabled)return;bool connected=WiFi.status()==WL_CONNECTED;
  if(!connected){if(networkWasConnected_){networkWasConnected_=false;udp_.stop();temporaryCoordinator_=false;masterReady_=false;reclaiming_=isMaster();masterMissingSince_=0;reclaimStableSince_=0;for(auto&p:peers_)p.online=false;}return;}
  if(!networkWasConnected_){networkWasConnected_=true;startedAt_=millis();lastHello_=lastSweep_=lastBootstrap_=0;temporaryCoordinator_=false;masterReady_=false;reclaiming_=isMaster();masterMissingSince_=0;reclaimStableSince_=0;udp_.begin(c_->federation.udpPort);sendHello();}
  receive();
  if(millis()-lastHello_>3000){lastHello_=millis();sendHello();}
  if(millis()-lastSweep_>1500){lastSweep_=millis();for(auto&p:peers_)p.online=(millis()-p.lastSeen)<10000;}
  if(millis()-lastBootstrap_>750){lastBootstrap_=millis();bootstrap();updateFailover();}
}

static void repairMojibakeInPlace(String& s){
  // Repariert die in Web-/XML-Quellen verbreitete Fehlinterpretation
  // "UTF-8 wurde als Windows-1252/Latin-1 gelesen und danach erneut als UTF-8 gespeichert".
  // Feste Durchläufe vermeiden eine zusätzliche Vollkopie des Strings pro Runde.
  for(int pass=0;pass<4;++pass){
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xA4","\xC3\xA4"); // doppelt fehlkodiertes ä
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xB6","\xC3\xB6"); // doppelt fehlkodiertes ö
    s.replace("\xC3\x83\xC6\x92\xC3\x82\xC2\xBC","\xC3\xBC"); // doppelt fehlkodiertes ü
    s.replace("\xC3\x83\xC2\xA4","\xC3\xA4"); // Ã¤ -> ä
    s.replace("\xC3\x83\xC2\xB6","\xC3\xB6"); // Ã¶ -> ö
    s.replace("\xC3\x83\xC2\xBC","\xC3\xBC"); // Ã¼ -> ü
    s.replace("\xC3\x83\xC2\x84","\xC3\x84"); // Ã„ -> Ä
    s.replace("\xC3\x83\xC2\x96","\xC3\x96"); // Ã– -> Ö
    s.replace("\xC3\x83\xC2\x9C","\xC3\x9C"); // Ãœ -> Ü
    s.replace("\xC3\x83\xC5\xB8","\xC3\x9F"); // ÃŸ -> ß
    s.replace("\xC3\x82\xC2\xB0","\xC2\xB0"); // Â° -> °
    s.replace("\xC3\x82\xC2\xA0"," ");        // Â  -> Leerzeichen
    s.replace("\xC3\xA2\xE2\x82\xAC\xE2\x80\x9C","\xE2\x80\x93"); // â€“ -> –
    s.replace("\xC3\xA2\xE2\x82\xAC\xE2\x80\x9D","\xE2\x80\x94"); // â€” -> —
  }
}
static String repairMojibake(String s){repairMojibakeInPlace(s);return s;}

static bool validUtf8(const String&in){
  const uint8_t*p=(const uint8_t*)in.c_str();size_t n=in.length();
  for(size_t i=0;i<n;){uint8_t c=p[i];if(c<0x80){i++;continue;}int need=0;uint32_t cp=0;if((c&0xE0)==0xC0){need=1;cp=c&0x1F;if(cp<2)return false;}else if((c&0xF0)==0xE0){need=2;cp=c&0x0F;}else if((c&0xF8)==0xF0){need=3;cp=c&0x07;}else return false;if(i+need>=n)return false;for(int k=1;k<=need;k++){uint8_t q=p[i+k];if((q&0xC0)!=0x80)return false;cp=(cp<<6)|(q&0x3F);}if((need==2&&cp<0x800)||(need==3&&cp<0x10000)||cp>0x10FFFF||(cp>=0xD800&&cp<=0xDFFF))return false;i+=need+1;}return true;
}
static void appendUtf8(String&out,uint32_t cp){
  if(cp<0x80)out+=(char)cp;else if(cp<0x800){out+=(char)(0xC0|(cp>>6));out+=(char)(0x80|(cp&0x3F));}else if(cp<0x10000){out+=(char)(0xE0|(cp>>12));out+=(char)(0x80|((cp>>6)&0x3F));out+=(char)(0x80|(cp&0x3F));}else{out+=(char)(0xF0|(cp>>18));out+=(char)(0x80|((cp>>12)&0x3F));out+=(char)(0x80|((cp>>6)&0x3F));out+=(char)(0x80|(cp&0x3F));}
}
static String singleByteToUtf8(const String&in){
  static const uint16_t cp1252[32]={0x20AC,0x0081,0x201A,0x0192,0x201E,0x2026,0x2020,0x2021,0x02C6,0x2030,0x0160,0x2039,0x0152,0x008D,0x017D,0x008F,0x0090,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,0x02DC,0x2122,0x0161,0x203A,0x0153,0x009D,0x017E,0x0178};
  String out;out.reserve(in.length()+16);for(size_t i=0;i<in.length();++i){uint8_t c=(uint8_t)in[i];uint32_t cp=c;if(c>=0x80&&c<=0x9F)cp=cp1252[c-0x80];appendUtf8(out,cp);}return out;
}

static bool bodyDeclaresLatin1(const String&body){
  int n=min((int)body.length(),220);
  String h=body.substring(0,n);h.toLowerCase();
  return h.indexOf("iso-8859-1")>=0 || h.indexOf("iso8859-1")>=0 || h.indexOf("windows-1252")>=0;
}
static void decodeEntitiesInPlace(String& v){
  v.replace("&amp;","&");v.replace("&quot;","\"");v.replace("&apos;","'");v.replace("&lt;","<");v.replace("&gt;",">");v.replace("&nbsp;"," ");
  v.replace("&auml;","\xC3\xA4");v.replace("&ouml;","\xC3\xB6");v.replace("&uuml;","\xC3\xBC");v.replace("&Auml;","\xC3\x84");v.replace("&Ouml;","\xC3\x96");v.replace("&Uuml;","\xC3\x9C");v.replace("&szlig;","\xC3\x9F");
  int p=0;
  while((p=v.indexOf("&#",p))>=0){int semi=v.indexOf(';',p+2);if(semi<0||semi-p>12){p+=2;continue;}String n=v.substring(p+2,semi);bool hex=n.startsWith("x")||n.startsWith("X");if(hex)n.remove(0,1);char*end=nullptr;uint32_t cp=strtoul(n.c_str(),&end,hex?16:10);if(!end||*end||cp==0||cp>0x10FFFF){p=semi+1;continue;}String u;appendUtf8(u,cp);String head=v.substring(0,p);String tail=v.substring(semi+1);v=head+u+tail;p+=u.length();}
  repairMojibakeInPlace(v);
}
static String decodeEntities(String v){decodeEntitiesInPlace(v);return v;}
static void stripMarkupInPlace(String& s){
  String out;out.reserve(s.length());bool tag=false;
  for(size_t i=0;i<s.length();++i){char c=s[i];if(c=='<'){tag=true;if(out.length()&&out[out.length()-1]!=' ')out+=' ';continue;}if(c=='>'){tag=false;continue;}if(!tag)out+=c;}
  decodeEntitiesInPlace(out);String clean;clean.reserve(out.length());bool ws=false;
  for(size_t i=0;i<out.length();++i){unsigned char c=(unsigned char)out[i];bool isWs=c==' '||c=='\t'||c=='\r'||c=='\n';if(isWs){if(!ws&&clean.length())clean+=' ';ws=true;}else{clean+=(char)c;ws=false;}}
  clean.trim();s=clean;
}
static String stripMarkup(String s){stripMarkupInPlace(s);return s;}
static String tagValue(const String&s,const String&tag,int start=0){
  String a="<"+tag;int p=s.indexOf(a,start);if(p<0)return"";p=s.indexOf('>',p);if(p<0)return"";
  int e=s.indexOf("</"+tag+">",p+1);if(e<0)return"";
  String v=s.substring(p+1,e);v.replace("<![CDATA[","");v.replace("]]>","");v.trim();return repairMojibake(decodeEntities(v));
}
bool NewsService::fetchFeed(const NewsFeed&f,const String&language,std::vector<NewsItem>&out,int limit,String&error,int&httpCode){
  std::unique_ptr<WiFiClient> client;bool tls=f.url.startsWith("https://");if(tls){auto*c=new WiFiClientSecure();c->setInsecure();client.reset(c);}else client.reset(new WiFiClient());
  HTTPClient h;h.setTimeout(3500);h.setConnectTimeout(2500);h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);h.setUserAgent(String("AntennaController/")+ANTCTRL_VERSION);
  if(!h.begin(*client,f.url)){error=(language=="en"?"Could not open feed: ":"Meldungsquelle konnte nicht geöffnet werden: ")+repairMojibake(f.name);return false;}
  int code=h.GET();httpCode=code;if(code!=HTTP_CODE_OK){error=repairMojibake(f.name)+": HTTP "+String(code);h.end();return false;}
  WiFiClient* stream=h.getStreamPtr();String body;body.reserve(32768);uint32_t started=millis();
  while((h.connected()||stream->available())&&millis()-started<3500&&body.length()<65536){while(stream->available()&&body.length()<65536)body+=(char)stream->read();delay(1);}h.end();
  if(body.isEmpty()){error=repairMojibake(f.name)+(language=="en"?": empty response":": leere Antwort");return false;}
  if(bodyDeclaresLatin1(body)&&!validUtf8(body))body=singleByteToUtf8(body);
  int pos=0,count=0;
  while(count<limit){
    int item=body.indexOf("<item",pos),entry=body.indexOf("<entry",pos);if(item<0||(entry>=0&&entry<item))item=entry;if(item<0)break;
    int end1=body.indexOf("</item>",item),end2=body.indexOf("</entry>",item);int finish=(end1>=0&&(end2<0||end1<end2))?end1:end2;if(finish<0)break;
    String chunk=body.substring(item,finish);String title=stripMarkup(tagValue(chunk,"title"));String link=tagValue(chunk,"link");String itemDate=tagValue(chunk,"pubDate");if(itemDate.isEmpty())itemDate=tagValue(chunk,"published");if(itemDate.isEmpty())itemDate=tagValue(chunk,"updated");
    if(link.isEmpty()){int lp=chunk.indexOf("<link");if(lp>=0){int hp=chunk.indexOf("href=\"",lp);if(hp>=0){hp+=6;int q=chunk.indexOf('"',hp);if(q>hp)link=chunk.substring(hp,q);}}}
    String summary=tagValue(chunk,"description");if(summary.isEmpty())summary=tagValue(chunk,"summary");if(summary.isEmpty())summary=tagValue(chunk,"content:encoded");if(summary.length()>4096)summary.remove(4096);stripMarkupInPlace(summary);if(summary.length()>420){summary.remove(417);summary+="...";}
    if(!title.isEmpty()){NewsItem ni;ni.title=repairMojibake(title);ni.summary=repairMojibake(summary);ni.link=repairMojibake(link);ni.sourceId=f.id;ni.source=repairMojibake(f.name);ni.date=repairMojibake(itemDate);out.push_back(ni);count++;}
    pos=finish+7;
  }
  if(count==0){error=repairMojibake(f.name)+(language=="en"?": no RSS/Atom items found":": keine RSS/Atom-Meldungen gefunden");return false;}return true;
}
void NewsService::begin(LocalConfig*c){c_=c;ensureExternalServiceMutex();if(!mutex_)mutex_=xSemaphoreCreateMutex();configChanged();}
void NewsService::configChanged(){if(!mutex_)mutex_=xSemaphoreCreateMutex();if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);if(c_){config_=c_->news;language_=c_->language;}if(language_.isEmpty())language_="de";lastFetch_=0;++generation_;if(!workerRunning_)fetching_=false;lastError_="";lastHttpCode_=0;items_.clear();sources_.clear();if(mutex_)xSemaphoreGive(mutex_);}
void NewsService::requestRefresh(){if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);lastFetch_=0;if(mutex_)xSemaphoreGive(mutex_);}
NewsSnapshot NewsService::snapshot()const{NewsSnapshot s;if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);s.items=items_;s.sources=sources_;s.lastError=lastError_;s.lastHttpCode=lastHttpCode_;s.lastSuccessEpoch=lastSuccessEpoch_;s.enabled=config_.enabled;s.fetching=fetching_;if(mutex_)xSemaphoreGive(mutex_);return s;}
void NewsService::workerThunk(void*arg){auto*self=static_cast<NewsService*>(arg);SemaphoreHandle_t gate=ensureExternalServiceMutex();if(gate)xSemaphoreTake(gate,portMAX_DELAY);NewsConfig cfg;String language;uint32_t generation;if(self->mutex_)xSemaphoreTake(self->mutex_,portMAX_DELAY);cfg=self->config_;language=self->language_;generation=self->generation_;if(self->mutex_)xSemaphoreGive(self->mutex_);self->fetchAll(cfg,language,generation);if(gate)xSemaphoreGive(gate);vTaskDelete(nullptr);}
void NewsService::loop(){if(WiFi.status()!=WL_CONNECTED||!mutex_)return;uint32_t now=millis();if(xSemaphoreTake(mutex_,portMAX_DELAY)!=pdTRUE)return;uint32_t intervalMinutes=std::max<uint16_t>(30,config_.refreshMinutes);uint32_t iv=intervalMinutes*60000UL;if(!config_.enabled||workerRunning_||(lastFetch_!=0&&now-lastFetch_<iv)){xSemaphoreGive(mutex_);return;}lastFetch_=now;workerRunning_=true;fetching_=true;xSemaphoreGive(mutex_);if(xTaskCreate(&NewsService::workerThunk,"news-fetch",8192,this,1,nullptr)!=pdPASS){xSemaphoreTake(mutex_,portMAX_DELAY);workerRunning_=false;fetching_=false;lastError_="Newsticker-Hintergrundabruf konnte nicht gestartet werden";xSemaphoreGive(mutex_);}}
void NewsService::fetchAll(const NewsConfig&cfg,const String&language,uint32_t generation){
  std::vector<NewsItem> newItems,previousItems;std::vector<NewsSourceStatus> newSources;String finalError,lastFailure;int finalHttpCode=0;uint32_t successEpoch=0;bool any=false;size_t totalCap=40;
  if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);previousItems=items_;if(mutex_)xSemaphoreGive(mutex_);
  if(cfg.enabled)for(const auto&f:cfg.feeds){
    if(!f.enabled||f.language!=cfg.language||f.url.isEmpty())continue;
    std::vector<NewsItem> bucket;String feedError;int feedCode=0;bool ok=fetchFeed(f,language,bucket,cfg.maxItems,feedError,feedCode);finalHttpCode=feedCode;
    NewsSourceStatus st;st.id=f.id;st.name=repairMojibake(f.name);st.ok=ok;st.httpCode=feedCode;st.itemCount=(int)bucket.size();st.error=ok?"":feedError;
    if(ok){any=true;for(auto&x:bucket){if(newItems.size()>=totalCap)break;newItems.push_back(x);}}
    else {if(!feedError.isEmpty())lastFailure=feedError;for(const auto&x:previousItems)if(x.sourceId==f.id&&newItems.size()<totalCap){newItems.push_back(x);st.stale=true;++st.itemCount;}}
    newSources.push_back(st);
    if(newItems.size()>=totalCap)break;
  }
  if(any)successEpoch=(uint32_t)time(nullptr);else if(!lastFailure.isEmpty())finalError=lastFailure;else finalError=language=="en"?"No active feed for the selected language":"Keine aktive Meldungsquelle für die gewählte Sprache";
  if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);if(generation==generation_){items_=std::move(newItems);sources_=std::move(newSources);lastHttpCode_=finalHttpCode;if(any)lastSuccessEpoch_=successEpoch;if(any)lastError_="";else lastError_=finalError;}fetching_=false;workerRunning_=false;if(mutex_)xSemaphoreGive(mutex_);
}

void WeatherService::begin(LocalConfig*c,Storage*store){c_=c;ensureExternalServiceMutex();store_=store;if(!mutex_)mutex_=xSemaphoreCreateMutex();configChanged();}
WeatherInfo WeatherService::info()const{WeatherInfo copy;if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);copy=info_;if(mutex_)xSemaphoreGive(mutex_);return copy;}
void WeatherService::configChanged(){
  if(!mutex_)mutex_=xSemaphoreCreateMutex();String postal=c_?c_->identity.postalCode:String();String language=c_?c_->language:String("de");String cachePostal,cachePlace;float cacheLat=0,cacheLon=0;if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);cachePostal=cachedPostalCode_;cachePlace=cachedPlace_;cacheLat=cachedLat_;cacheLon=cachedLon_;if(mutex_)xSemaphoreGive(mutex_);
  if(cachePostal!=postal){cachePostal="";cachePlace="";cacheLat=0;cacheLon=0;float lat=0,lon=0;String place;if(store_&&store_->loadWeatherLocation(postal,lat,lon,place)){cachePostal=postal;cacheLat=lat;cacheLon=lon;cachePlace=place;}}
  if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);WeatherInfo previous=info_;postalCode_=postal;language_=language;cachedPostalCode_=cachePostal;cachedLat_=cacheLat;cachedLon_=cacheLon;cachedPlace_=cachePlace;lastFetch_=0;++generation_;info_=previous.valid&&previous.postalCode==postal?previous:WeatherInfo{};info_.postalCode=postal;info_.enabled=!postal.isEmpty();if(workerRunning_)info_.fetching=true;if(mutex_)xSemaphoreGive(mutex_);
}
bool WeatherService::geocode(const String&postal,const String&language,float&lat,float&lon,String&place,String&error){
  const bool en=language=="en";
  // 1) Primär: Open-Meteo-Geocoding. Laut API-Dokumentation darf name eine PLZ sein;
  //    countryCode=DE begrenzt die Treffer fest auf Deutschland.
  {
    WiFiClientSecure client;client.setInsecure();HTTPClient h;h.setTimeout(9000);h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);h.setUserAgent(String("AntennaController/")+ANTCTRL_VERSION);
    String url="https://geocoding-api.open-meteo.com/v1/search?name="+postal+"&count=10&language="+(en?"en":"de")+"&format=json&countryCode=DE";
    if(h.begin(client,url)){
      int code=h.GET();
      if(code==200){
        String body=h.getString();JsonDocument d;
        if(!deserializeJson(d,body)){
          JsonArray results=d["results"].as<JsonArray>();
          if(!results.isNull()){
            for(JsonObject r:results){
              bool exact=false;JsonArray pcs=r["postcodes"].as<JsonArray>();
              if(!pcs.isNull())for(JsonVariant pc:pcs){const char*v=pc.as<const char*>();if(v&&String(v)==postal){exact=true;break;}}
              // Bei einer numerischen PLZ-Suche akzeptieren wir nur den exakten PLZ-Treffer.
              if(!exact)continue;
              lat=r["latitude"]|0.0f;lon=r["longitude"]|0.0f;
              place=String((const char*)(r["name"]|postal.c_str()));String admin=String((const char*)(r["admin1"]|""));if(!admin.isEmpty()&&admin!=place)place+=" · "+admin;
              h.end();place=repairMojibake(place);return lat>=-90.0f&&lat<=90.0f&&lon>=-180.0f&&lon<=180.0f&&(lat!=0.0f||lon!=0.0f);
            }
          }
        }
      }
      h.end();
    }
  }

  // 2) Fallback ausschließlich für die PLZ->Koordinaten-Auflösung:
  //    Zippopotam.us stellt eine direkte Länder-/PLZ-Abfrage bereit. Für Deutschland
  //    ist der Pfad /DE/<PLZ>. Wetterdaten kommen weiterhin von Open-Meteo/DWD ICON.
  {
    WiFiClientSecure client;client.setInsecure();HTTPClient h;h.setTimeout(9000);h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);h.setUserAgent(String("AntennaController/")+ANTCTRL_VERSION);
    String url="https://api.zippopotam.us/DE/"+postal;
    if(h.begin(client,url)){
      int code=h.GET();
      if(code==200){
        String body=h.getString();JsonDocument d;
        if(!deserializeJson(d,body)){
          JsonArray places=d["places"].as<JsonArray>();
          if(!places.isNull()&&places.size()){
            JsonObject r=places[0].as<JsonObject>();
            lat=String((const char*)(r["latitude"]|"0")).toFloat();lon=String((const char*)(r["longitude"]|"0")).toFloat();
            place=String((const char*)(r["place name"]|postal.c_str()));
            String state=String((const char*)(r["state"]|""));if(!state.isEmpty()&&state!=place)place+=" · "+state;
            h.end();place=repairMojibake(place);return lat>=-90.0f&&lat<=90.0f&&lon>=-180.0f&&lon<=180.0f&&(lat!=0.0f||lon!=0.0f);
          }
        }
      }
      h.end();
    }
  }

  error=en?"German postal code not found":"Deutsche Postleitzahl nicht gefunden";
  return false;
}
bool WeatherService::forecast(float lat,float lon,const String&place,const String&language,WeatherInfo&result){
  const String query="latitude="+String(lat,5)+"&longitude="+String(lon,5)+"&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,wind_speed_10m,wind_direction_10m,wind_gusts_10m&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max,weather_code&timezone=auto&forecast_days=1";
  String body;int code=-1;
  auto getForecast=[&](const String&base)->bool{WiFiClientSecure client;client.setInsecure();HTTPClient h;h.setTimeout(9000);h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);h.setUserAgent(String("AntennaController/")+ANTCTRL_VERSION);String url=base+"?"+query;if(!h.begin(client,url))return false;code=h.GET();body=h.getString();h.end();return code==200;};
  // DWD ICON bleibt erste Wahl. Falls der spezialisierte Endpunkt eine Anfrage
  // ablehnt, fällt die Anzeige automatisch auf Open-Meteos allgemeinen Forecast
  // zurück, statt bis zum nächsten Firmware-Update dauerhaft HTTP 400 zu zeigen.
  if(!getForecast("https://api.open-meteo.com/v1/dwd-icon")){
    int dwdCode=code;
    if(!getForecast("https://api.open-meteo.com/v1/forecast")){String reason;if(!body.isEmpty()){JsonDocument ed;if(!deserializeJson(ed,body))reason=String((const char*)(ed["reason"]|""));}result.error=(language=="en"?"Weather service HTTP ":"Wetterdienst HTTP ")+String(code)+(reason.isEmpty()?String():String(" · ")+reason);if(code<0&&dwdCode>0)result.error+=(String(" · DWD HTTP ")+String(dwdCode));return false;}
  }
  JsonDocument d;if(deserializeJson(d,body)){result.error=language=="en"?"Invalid weather response":"Ungültige Wetterantwort";return false;}
  JsonObject cur=d["current"].as<JsonObject>();if(cur.isNull()){result.error=language=="en"?"Current weather is missing":"Aktuelles Wetter fehlt";return false;}
  result.place=place;result.temperature=cur["temperature_2m"]|0.0f;result.apparentTemperature=cur["apparent_temperature"]|0.0f;result.humidity=cur["relative_humidity_2m"]|0;result.precipitation=cur["precipitation"]|0.0f;result.weatherCode=cur["weather_code"]|-1;result.windSpeed=cur["wind_speed_10m"]|0.0f;result.windDirection=cur["wind_direction_10m"]|0;result.windGusts=cur["wind_gusts_10m"]|0.0f;
  JsonObject daily=d["daily"].as<JsonObject>();JsonArray maxA=daily["temperature_2m_max"].as<JsonArray>();JsonArray minA=daily["temperature_2m_min"].as<JsonArray>();JsonArray ppA=daily["precipitation_probability_max"].as<JsonArray>();JsonArray wcA=daily["weather_code"].as<JsonArray>();if(maxA.size())result.todayMax=maxA[0].as<float>();if(minA.size())result.todayMin=minA[0].as<float>();if(ppA.size())result.todayPrecipitationProbability=ppA[0].as<int>();if(wcA.size())result.todayWeatherCode=wcA[0].as<int>();return true;
}
void WeatherService::workerThunk(void*arg){auto*self=static_cast<WeatherService*>(arg);SemaphoreHandle_t gate=ensureExternalServiceMutex();if(gate)xSemaphoreTake(gate,portMAX_DELAY);self->fetchNow();if(gate)xSemaphoreGive(gate);vTaskDelete(nullptr);}
void WeatherService::fetchNow(){
  String postal,language,cachePostal,place;float lat=0,lon=0;uint32_t generation=0;WeatherInfo result,previous;if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);postal=postalCode_;language=language_;cachePostal=cachedPostalCode_;place=cachedPlace_;lat=cachedLat_;lon=cachedLon_;generation=generation_;result=info_;previous=info_;if(mutex_)xSemaphoreGive(mutex_);
  result.enabled=!postal.isEmpty();result.postalCode=postal;result.fetching=false;result.error="";result.valid=false;bool geocoded=false;bool cachedValid=cachePostal==postal&&!place.isEmpty()&&lat>=-90.0f&&lat<=90.0f&&lon>=-180.0f&&lon<=180.0f&&(lat!=0.0f||lon!=0.0f);
  if(!postal.isEmpty()){
    if(!cachedValid){if(!geocode(postal,language,lat,lon,place,result.error))goto commit;geocoded=true;}
    result.latitude=lat;result.longitude=lon;result.valid=forecast(lat,lon,place,language,result);if(result.valid)result.lastSuccessEpoch=(uint32_t)time(nullptr);
  }
commit:
  if(!result.valid&&result.enabled&&result.postalCode==postal&&previous.valid&&previous.postalCode==postal){String fetchError=result.error;result=previous;result.error=fetchError;}
  result.fetching=false;if(mutex_)xSemaphoreTake(mutex_,portMAX_DELAY);if(generation==generation_){info_=result;if(geocoded){cachedPostalCode_=postal;cachedLat_=lat;cachedLon_=lon;cachedPlace_=place;pendingLocationSave_=true;pendingPostal_=postal;pendingLat_=lat;pendingLon_=lon;pendingPlace_=place;}}else info_.fetching=false;workerRunning_=false;if(mutex_)xSemaphoreGive(mutex_);
}
void WeatherService::loop(){
  if(!mutex_)return;
  bool save=false;String savedPostal,savedPlace;float savedLat=0,savedLon=0;
  xSemaphoreTake(mutex_,portMAX_DELAY);if(pendingLocationSave_){save=true;savedPostal=pendingPostal_;savedPlace=pendingPlace_;savedLat=pendingLat_;savedLon=pendingLon_;pendingLocationSave_=false;}xSemaphoreGive(mutex_);
  if(save&&store_)store_->saveWeatherLocation(savedPostal,savedLat,savedLon,savedPlace);
  if(WiFi.status()!=WL_CONNECTED)return;
  uint32_t now=millis();xSemaphoreTake(mutex_,portMAX_DELAY);uint32_t interval=30UL*60UL*1000UL;if(postalCode_.isEmpty()||workerRunning_||(lastFetch_!=0&&now-lastFetch_<interval)){xSemaphoreGive(mutex_);return;}lastFetch_=now;workerRunning_=true;info_.enabled=true;info_.fetching=true;xSemaphoreGive(mutex_);
  if(xTaskCreate(&WeatherService::workerThunk,"weather-fetch",8192,this,1,nullptr)!=pdPASS){xSemaphoreTake(mutex_,portMAX_DELAY);workerRunning_=false;info_.fetching=false;info_.error="Wetter-Hintergrundabruf konnte nicht gestartet werden";xSemaphoreGive(mutex_);}
}
