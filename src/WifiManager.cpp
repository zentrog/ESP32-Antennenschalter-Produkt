#include "WifiManager.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_arduino_version.h>
#include <cstring>

static constexpr uint32_t ConnectTimeoutMs = 8000;
static constexpr uint32_t BootHoldMs = 3000;
static constexpr int BootButtonGpio = 0;
static constexpr uint32_t ProvMagic = 0x31564F52UL; // "ROV1" on the wire, local protocol marker only
static constexpr uint8_t ProvVersion = 2;
static constexpr uint8_t ProvProbe = 1;
static constexpr uint8_t ProvOffer = 2;
static constexpr uint8_t ProvRequest = 3;
static constexpr uint8_t ProvCredentials = 4;
static constexpr uint8_t AdmissionOff = 0;
static constexpr uint8_t AdmissionAsk = 1;
static constexpr uint8_t AdmissionAuto = 2;
static constexpr uint8_t AssignNone = 0;
static constexpr uint8_t AssignJoin = 1;
static constexpr uint8_t AssignOwn = 2;
static constexpr uint8_t AssignWifi = 3;
static const uint8_t BroadcastMac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

struct __attribute__((packed)) ProvHeader {
  uint32_t magic;
  uint8_t version;
  uint8_t type;
};
struct __attribute__((packed)) ProvProbeMsg {
  ProvHeader h;
  uint32_t nonce;
  char controllerId[20];
  char boardProfile[32];
  char systemId[33];
  char masterId[20];
  char role[12];
};
struct __attribute__((packed)) ProvOfferMsg {
  ProvHeader h;
  uint32_t nonce;
  uint8_t admission;
  uint8_t assignment;
  char masterId[20];
  char systemName[40];
};
struct __attribute__((packed)) ProvRequestMsg {
  ProvHeader h;
  uint32_t nonce;
  uint8_t assignment;
  char controllerId[20];
};
struct __attribute__((packed)) ProvCredentialsMsg {
  ProvHeader h;
  uint32_t nonce;
  uint8_t assignment;
  uint16_t failoverPriority;
  char ssid[33];
  char password[65];
  char systemId[33];
  char systemName[40];
  char masterId[20];
};
static_assert(sizeof(ProvProbeMsg) <= 250, "ESP-NOW probe packet too large");
static_assert(sizeof(ProvOfferMsg) <= 250, "ESP-NOW offer packet too large");
static_assert(sizeof(ProvRequestMsg) <= 250, "ESP-NOW request packet too large");
static_assert(sizeof(ProvCredentialsMsg) <= 250, "ESP-NOW provisioning packet too large");

static WifiManager* gWifiManager = nullptr;

#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void espNowReceiveThunk(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if(gWifiManager && info) gWifiManager->onEspNowPacket(info->src_addr, data, len);
}
#else
static void espNowReceiveThunk(const uint8_t* mac, const uint8_t* data, int len) {
  if(gWifiManager) gWifiManager->onEspNowPacket(mac, data, len);
}
#endif

static void copyText(char* dst, size_t size, const String& value) {
  if(!dst || size==0) return;
  size_t n = value.length();
  if(n >= size) n = size-1;
  memcpy(dst, value.c_str(), n);
  dst[n] = 0;
}
static String textFrom(const char* p, size_t size) {
  if(!p || !size) return String();
  size_t n=0;while(n<size && p[n])++n;
  String out;out.reserve(n);
  for(size_t i=0;i<n;++i)out+=(char)p[i];
  return out;
}
static void setHeader(ProvHeader& h, uint8_t type) {
  h.magic=ProvMagic;
  h.version=ProvVersion;
  h.type=type;
}
static String macText(const uint8_t* mac) {
  char b[18];snprintf(b,sizeof(b),"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);return String(b);
}
static String makeProvisionedSystemId() {
  char out[33];
  uint32_t a=esp_random(),b=esp_random(),c=esp_random(),d=esp_random();
  snprintf(out,sizeof(out),"%08lX%08lX%08lX%08lX",(unsigned long)a,(unsigned long)b,(unsigned long)c,(unsigned long)d);
  return String(out);
}
static uint8_t admissionCode(const String& mode) {
  return mode=="auto"?AdmissionAuto:(mode=="ask"?AdmissionAsk:AdmissionOff);
}
static uint8_t assignmentCode(const String& mode) {
  return mode=="own"?AssignOwn:(mode=="join"?AssignJoin:(mode=="wifi"?AssignWifi:AssignNone));
}
static String assignmentName(uint8_t mode) {
  return mode==AssignOwn?String("own"):(mode==AssignJoin?String("join"):(mode==AssignWifi?String("wifi"):String()));
}

void WifiManager::begin(Storage* store, LocalConfig* config) {
  store_ = store;
  config_ = config;
  gWifiManager = this;
  store_->ensureDefaultWifi();
  pinMode(BootButtonGpio, INPUT_PULLUP);

  if (store_->consumeForceSetup()) {
    startSetupPortal(false);
    return;
  }

  if (connectKnownNetworks()) {
    beginEspNow();
    return;
  }

  // Already-assigned controllers have nothing to provision. Enter recovery immediately
  // instead of waiting for an unrelated ESP-NOW offer before the web server can start.
  if (config_ && config_->federation.role != "unassigned" && !config_->federation.systemId.isEmpty()) {
    startSetupPortal(true);
    return;
  }

  // A controller with no usable WLAN first looks for an already running AntennaController master.
  // If no master exists, the familiar setup AP is opened as fallback for the first controller.
  if (tryEspNowProvisioning()) {
    setupMode_ = false;
    beginEspNow();
    return;
  }

  startSetupPortal(true);
}

bool WifiManager::networkVisible(const String& ssid, int scanCount) const {
  for (int i = 0; i < scanCount; ++i) if (WiFi.SSID(i) == ssid) return true;
  return false;
}

bool WifiManager::connectOne(const WifiCredential& cred, uint32_t timeoutMs) {
  WiFi.disconnect(false, false);
  delay(80);
  WiFi.begin(cred.ssid.c_str(), cred.password.c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::connectKnownNetworks() {
  const auto nets = store_->wifiNetworks();
  if (nets.empty()) return false;

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  int found = WiFi.scanNetworks(false, true);
  if (found < 0) found = 0;

  // Priority is the vector order: first saved network wins.
  // Only networks explicitly saved by the user or provisioned by an authorized master are considered.
  // A single incomplete startup scan must never suppress the highest-priority WLAN.
  for (size_t i = 0; i < nets.size(); ++i) {
    const auto& net = nets[i];
    const bool highestPriority = (i == 0);
    if (!highestPriority && found > 0 && !networkVisible(net.ssid, found)) continue;
    Serial.println("WLAN: verbinde Prioritaet " + String(i + 1) + " -> " + net.ssid);
    if (connectOne(net, ConnectTimeoutMs)) {
      WiFi.scanDelete();
      setupMode_ = false;
      Serial.println("WLAN: verbunden -> " + net.ssid + "  IP: " + WiFi.localIP().toString());
      return true;
    }
    Serial.println("WLAN: Verbindung fehlgeschlagen -> " + net.ssid);
  }
  WiFi.scanDelete();
  return false;
}

void WifiManager::startSetupPortal(bool retrySavedNetworks) {
  provisioningWait_ = false;
  endEspNow();
  setupMode_ = true;
  fallbackRecoveryMode_ = retrySavedNetworks;
  fallbackConnecting_ = false;
  fallbackNetworkIndex_ = 0;
  fallbackNextTryAt_ = millis() + 5000;
  fallbackConnectStartedAt_ = 0;
  fallbackConnectedAt_ = 0;
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_AP_STA);
  uint64_t chip = ESP.getEfuseMac();
  char id[7];
  snprintf(id, sizeof(id), "%06llX", (unsigned long long)(chip & 0xFFFFFFULL));
  setupSsid_ = "AntennaController-" + String(id);
  WiFi.softAP(setupSsid_.c_str());
  dns_.start(53, "*", WiFi.softAPIP());
  Serial.println("Setup-AP: " + setupSsid_ + "  http://192.168.4.1/");
}

void WifiManager::processDns() {
  if (setupMode_) dns_.processNextRequest();
}

void WifiManager::requestSetupMode() {
  if (!store_) return;
  store_->setForceSetup();
}

bool WifiManager::testAndAddNetwork(const String& ssid, const String& password, String& error, bool english) {
  auto msg=[&](const char* de,const char* en){return english?String(en):String(de);};
  if (!store_) { error = msg("Speicher nicht bereit","Storage is not ready"); return false; }
  String clean = ssid;
  clean.trim();
  if (clean.isEmpty() || clean.length() > 32) { error = msg("SSID muss 1..32 Zeichen lang sein","SSID must contain 1..32 characters"); return false; }
  if (password.length() > 63) { error = msg("WLAN-Passwort ist zu lang","Wi-Fi password is too long"); return false; }

  WifiCredential previous;
  bool hadPrevious = false;
  if (WiFi.status() == WL_CONNECTED && !setupMode_) {
    String active = WiFi.SSID();
    for (const auto& n : store_->wifiNetworks()) if (n.ssid == active) { previous=n; hadPrevious=true; break; }
  }

  WifiCredential candidate{clean, password};
  if (!setupMode_) WiFi.mode(WIFI_AP_STA);

  if (!connectOne(candidate, 12000)) {
    WiFi.disconnect(false, false);
    if(hadPrevious) connectOne(previous, ConnectTimeoutMs);
    error = msg("Verbindung fehlgeschlagen. SSID/Passwort und 2,4-GHz-Verfügbarkeit prüfen.","Connection failed. Check SSID/password and 2.4 GHz availability.");
    return false;
  }

  String storeError;
  if (!store_->addOrUpdateWifi(clean, password, true, storeError)) {
    if(hadPrevious) connectOne(previous, ConnectTimeoutMs);
    error=english?"Wi-Fi settings could not be saved":storeError;
    return false;
  }
  return true;
}

int WifiManager::scanNetworks() {
  if (WiFi.getMode() != WIFI_AP_STA && setupMode_) WiFi.mode(WIFI_AP_STA);
  WiFi.scanDelete();
  return WiFi.scanNetworks(false, true);
}

bool WifiManager::beginEspNow() {
  if(espNowReady_) return true;
  if(WiFi.getMode()==WIFI_OFF) WiFi.mode(WIFI_STA);
  if(esp_now_init()!=ESP_OK) return false;
  if(esp_now_register_recv_cb(espNowReceiveThunk)!=ESP_OK){esp_now_deinit();return false;}
  espNowReady_=true;
  return ensureEspNowPeer(BroadcastMac);
}

void WifiManager::endEspNow() {
  if(!espNowReady_) return;
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  espNowReady_=false;
}

bool WifiManager::ensureEspNowPeer(const uint8_t* mac) {
  if(!espNowReady_||!mac) return false;
  if(esp_now_is_peer_exist(mac)) return true;
  esp_now_peer_info_t p{};
  memcpy(p.peer_addr,mac,6);
  p.channel=0;
  p.ifidx=WIFI_IF_STA;
  p.encrypt=false;
  esp_err_t r=esp_now_add_peer(&p);
  return r==ESP_OK || r==ESP_ERR_ESPNOW_EXIST;
}

ProvisioningCandidate* WifiManager::findCandidate(const String& controllerId, const uint8_t* mac) {
  for(auto& c:candidates_){
    if(!controllerId.isEmpty()&&c.controllerId==controllerId) return &c;
    if(mac&&memcmp(c.mac,mac,6)==0) return &c;
  }
  return nullptr;
}

void WifiManager::onEspNowPacket(const uint8_t* mac, const uint8_t* data, int len) {
  if(!mac||!data||len<(int)sizeof(ProvHeader)) return;
  ProvHeader h{};memcpy(&h,data,sizeof(h));
  if(h.magic!=ProvMagic||h.version!=ProvVersion) return;

  if(h.type==ProvProbe && len==(int)sizeof(ProvProbeMsg)){
    if(!config_||config_->federation.role!="master"||config_->federation.systemId.isEmpty()) return;
    ProvProbeMsg m{};memcpy(&m,data,sizeof(m));
    memcpy(pendingProbe_.mac,mac,6);
    memcpy(pendingProbe_.controllerId,m.controllerId,sizeof(m.controllerId));
    memcpy(pendingProbe_.boardProfile,m.boardProfile,sizeof(m.boardProfile));
    memcpy(pendingProbe_.systemId,m.systemId,sizeof(m.systemId));
    memcpy(pendingProbe_.masterId,m.masterId,sizeof(m.masterId));
    memcpy(pendingProbe_.role,m.role,sizeof(m.role));
    pendingProbe_.controllerId[sizeof(pendingProbe_.controllerId)-1]=0;
    pendingProbe_.boardProfile[sizeof(pendingProbe_.boardProfile)-1]=0;
    pendingProbe_.systemId[sizeof(pendingProbe_.systemId)-1]=0;
    pendingProbe_.masterId[sizeof(pendingProbe_.masterId)-1]=0;
    pendingProbe_.role[sizeof(pendingProbe_.role)-1]=0;
    pendingProbe_.nonce=m.nonce;
    pendingProbe_.channel=(uint8_t)WiFi.channel();
    pendingProbeReady_=true;
    return;
  }

  if(h.type==ProvOffer && provisioningWait_ && len==(int)sizeof(ProvOfferMsg)){
    ProvOfferMsg m{};memcpy(&m,data,sizeof(m));
    if(m.nonce!=activeProvisionNonce_) return;
    size_t count=offerCount_;
    for(size_t i=0;i<count&&i<8;i++) if(memcmp(offers_[i].mac,mac,6)==0){offers_[i].admission=m.admission;offers_[i].assignment=m.assignment;offers_[i].channel=(uint8_t)WiFi.channel();return;}
    if(count<8){OfferRecord o{};memcpy(o.mac,mac,6);o.nonce=m.nonce;o.channel=(uint8_t)WiFi.channel();o.admission=m.admission;o.assignment=m.assignment;offers_[count]=o;offerCount_=count+1;}
    return;
  }

  if(h.type==ProvRequest && len==(int)sizeof(ProvRequestMsg)){
    if(!config_||config_->federation.role!="master"||config_->federation.systemId.isEmpty()) return;
    ProvRequestMsg m{};memcpy(&m,data,sizeof(m));
    memcpy(pendingRequest_.mac,mac,6);
    memcpy(pendingRequest_.controllerId,m.controllerId,sizeof(m.controllerId));
    pendingRequest_.controllerId[sizeof(pendingRequest_.controllerId)-1]=0;
    pendingRequest_.nonce=m.nonce;
    pendingRequest_.assignment=m.assignment;
    pendingRequestReady_=true;
    return;
  }

  if(h.type==ProvCredentials && provisioningWait_ && len==(int)sizeof(ProvCredentialsMsg)){
    ProvCredentialsMsg m{};memcpy(&m,data,sizeof(m));
    if(m.nonce!=activeProvisionNonce_) return;
    memcpy(credentialBuffer_,data,sizeof(m));
    credentialLength_=sizeof(m);
    credentialReady_=true;
  }
}

bool WifiManager::sendOffer(const uint8_t* mac, uint32_t nonce, uint8_t assignment) {
  if(!ensureEspNowPeer(mac)||!config_) return false;
  ProvOfferMsg m{};setHeader(m.h,ProvOffer);m.nonce=nonce;m.admission=admissionCode(config_->federation.admissionMode);m.assignment=assignment;
  copyText(m.masterId,sizeof(m.masterId),config_->controllerId);
  copyText(m.systemName,sizeof(m.systemName),config_->federation.systemName);
  return esp_now_send(mac,(const uint8_t*)&m,sizeof(m))==ESP_OK;
}

void WifiManager::processPendingProbe() {
  if(!pendingProbeReady_) return;
  PendingProbe p=pendingProbe_;pendingProbeReady_=false;
  String id=String(p.controllerId);if(id.isEmpty()||!config_||id==config_->controllerId)return;
  String probeSystem=String(p.systemId),probeMaster=String(p.masterId),probeRole=String(p.role);
  bool recovery=probeRole=="follower"&&!probeSystem.isEmpty()&&probeSystem==config_->federation.systemId&&probeMaster==config_->controllerId;
  ProvisioningCandidate* c=findCandidate(id,p.mac);
  if(!c){ProvisioningCandidate x;x.controllerId=id;x.boardProfile=String(p.boardProfile);x.macAddress=macText(p.mac);memcpy(x.mac,p.mac,6);x.authorizedMode="";candidates_.push_back(x);c=&candidates_.back();}
  c->boardProfile=String(p.boardProfile);c->macAddress=macText(p.mac);memcpy(c->mac,p.mac,6);c->lastSeen=millis();c->nonce=p.nonce;c->online=true;c->recovery=recovery;
  if(recovery)c->authorizedMode="join";
  sendOffer(p.mac,p.nonce,assignmentCode(c->authorizedMode));
}

String WifiManager::currentWifiPassword() const {
  if(!store_||WiFi.status()!=WL_CONNECTED) return String();
  String current=WiFi.SSID();
  for(const auto& n:store_->wifiNetworks()) if(n.ssid==current) return n.password;
  return String();
}

bool WifiManager::sendCredentials(const ProvisioningCandidate& candidate, uint8_t assignment, uint32_t nonce, String& error) {
  if(!config_||!store_||WiFi.status()!=WL_CONNECTED){error="WLAN nicht bereit";return false;}
  String ssid=WiFi.SSID();String password=currentWifiPassword();
  bool stored=false;for(const auto& n:store_->wifiNetworks())if(n.ssid==ssid){stored=true;break;}
  if(!stored){error=config_->language=="en"?"Current Wi-Fi credential is not stored on the master":"Das aktuell verwendete WLAN ist auf dem Master nicht mit Zugangsdaten gespeichert";return false;}
  if(!ensureEspNowPeer(candidate.mac)){error="ESP-NOW Peer konnte nicht angelegt werden";return false;}
  ProvCredentialsMsg m{};setHeader(m.h,ProvCredentials);m.nonce=nonce;m.assignment=assignment;m.failoverPriority=config_->federation.failoverPriority;
  copyText(m.ssid,sizeof(m.ssid),ssid);copyText(m.password,sizeof(m.password),password);
  if(assignment==AssignJoin){
    m.failoverPriority=store_->allocateFailoverPriority();
    copyText(m.systemId,sizeof(m.systemId),config_->federation.systemId);copyText(m.systemName,sizeof(m.systemName),config_->federation.systemName);copyText(m.masterId,sizeof(m.masterId),config_->controllerId);
  }
  if(esp_now_send(candidate.mac,(const uint8_t*)&m,sizeof(m))!=ESP_OK){error="ESP-NOW Senden fehlgeschlagen";return false;}
  return true;
}

void WifiManager::processPendingRequest() {
  if(!pendingRequestReady_) return;
  PendingRequest r=pendingRequest_;pendingRequestReady_=false;
  if(!config_||config_->federation.role!="master")return;
  String id=String(r.controllerId);ProvisioningCandidate* c=findCandidate(id,r.mac);if(!c||!c->online||c->nonce!=r.nonce)return;
  bool allowed=false;
  String requested=assignmentName(r.assignment);
  if(r.assignment==AssignJoin&&config_->federation.admissionMode=="auto"&&c->authorizedMode.isEmpty())allowed=true;
  if(!c->authorizedMode.isEmpty()&&c->authorizedMode==requested)allowed=true;
  if(!allowed)return;
  String e;if(!sendCredentials(*c,r.assignment,r.nonce,e)){if(store_)store_->addError("ESPNOW_PROVISION",e);return;}
  c->authorizedMode="";
}

void WifiManager::pruneProvisioningCandidates() {
  uint32_t now=millis();
  for(auto& c:candidates_)c.online=(now-c.lastSeen)<30000;
  if(candidates_.size()>12){candidates_.erase(candidates_.begin(),candidates_.begin()+(candidates_.size()-12));}
}

void WifiManager::processEspNow() {
  processPendingProbe();
  processPendingRequest();
  if(millis()-lastCandidateSweep_>2000){lastCandidateSweep_=millis();pruneProvisioningCandidates();}
}

std::vector<ProvisioningCandidate> WifiManager::provisioningCandidates() const {
  std::vector<ProvisioningCandidate> out;
  uint32_t now=millis();
  for(const auto& c:candidates_)if((now-c.lastSeen)<30000)out.push_back(c);
  return out;
}

bool WifiManager::authorizeProvisioning(const String& controllerId, const String& mode, String& error) {
  if(!config_||config_->federation.role!="master"||config_->federation.systemId.isEmpty()){error=config_&&config_->language=="en"?"Only the master can provision new controllers":"Nur der Master kann neue Controller provisionieren";return false;}
  if(config_->federation.admissionMode=="off"){error=config_->language=="en"?"Admission of new controllers is disabled":"Aufnahme neuer Controller ist ausgeschaltet";return false;}
  if(mode!="join"&&mode!="own"&&mode!="wifi"){error=config_->language=="en"?"Invalid provisioning action":"Ungültige Provisionierungsaktion";return false;}
  ProvisioningCandidate* c=findCandidate(controllerId);if(!c||!c->online){error=config_->language=="en"?"Controller is no longer directly reachable":"Controller ist direkt nicht mehr erreichbar";return false;}
  c->authorizedMode=mode;
  return true;
}

bool WifiManager::sendProvisionProbe(uint8_t channel, uint32_t nonce) {
  if(!config_||!espNowReady_)return false;
  esp_wifi_set_channel(channel,WIFI_SECOND_CHAN_NONE);
  if(!ensureEspNowPeer(BroadcastMac))return false;
  ProvProbeMsg m{};setHeader(m.h,ProvProbe);m.nonce=nonce;copyText(m.controllerId,sizeof(m.controllerId),config_->controllerId);copyText(m.boardProfile,sizeof(m.boardProfile),config_->boardProfile);copyText(m.systemId,sizeof(m.systemId),config_->federation.systemId);copyText(m.masterId,sizeof(m.masterId),config_->federation.permanentMasterId);copyText(m.role,sizeof(m.role),config_->federation.role);
  return esp_now_send(BroadcastMac,(const uint8_t*)&m,sizeof(m))==ESP_OK;
}

bool WifiManager::requestProvisioning(const OfferRecord& offer, uint8_t assignment) {
  if(!config_||assignment==AssignNone)return false;
  esp_wifi_set_channel(offer.channel,WIFI_SECOND_CHAN_NONE);
  if(!ensureEspNowPeer(offer.mac))return false;
  credentialReady_=false;credentialLength_=0;
  ProvRequestMsg m{};setHeader(m.h,ProvRequest);m.nonce=offer.nonce;m.assignment=assignment;copyText(m.controllerId,sizeof(m.controllerId),config_->controllerId);
  if(esp_now_send(offer.mac,(const uint8_t*)&m,sizeof(m))!=ESP_OK)return false;
  uint32_t start=millis();while(!credentialReady_&&millis()-start<3000)delay(20);
  return credentialReady_;
}

bool WifiManager::applyReceivedCredentials(String& error) {
  if(!credentialReady_||credentialLength_!=(int)sizeof(ProvCredentialsMsg)||!store_||!config_)return false;
  ProvCredentialsMsg m{};memcpy(&m,credentialBuffer_,sizeof(m));credentialReady_=false;
  String ssid=textFrom(m.ssid,sizeof(m.ssid));String password=textFrom(m.password,sizeof(m.password));
  if(ssid.isEmpty()){error="Leere WLAN-SSID empfangen";return false;}
  if(!store_->addOrUpdateWifi(ssid,password,true,error))return false;

  LocalConfig n=*config_;
  if(m.assignment==AssignJoin){
    String systemId=textFrom(m.systemId,sizeof(m.systemId)),masterId=textFrom(m.masterId,sizeof(m.masterId));
    if(systemId.isEmpty()||masterId.isEmpty()){error="Ungültige Anlagenzuordnung empfangen";return false;}
    n.federation.systemId=systemId;n.federation.systemName=textFrom(m.systemName,sizeof(m.systemName));if(n.federation.systemName.isEmpty())n.federation.systemName="Antennenanlage";n.federation.role="follower";n.federation.permanentMasterId=masterId;n.federation.admissionMode="off";n.federation.failoverPriority=m.failoverPriority<100?100:m.failoverPriority;
  }else if(m.assignment==AssignOwn){
    n.federation.systemId=makeProvisionedSystemId();n.federation.systemName="Antennenanlage";n.federation.role="master";n.federation.permanentMasterId=n.controllerId;n.federation.admissionMode="ask";n.federation.failoverPriority=0;
  }else if(m.assignment==AssignWifi){
    // WLAN provisioning alone deliberately leaves an unassigned controller unassigned.
    // Existing system membership is preserved for recovery/migration scenarios.
  }else{error="Ungültiger Provisionierungsmodus";return false;}
  n.schema=7;
  if(!store_->saveLocal(n,error))return false;
  *config_=n;if(m.assignment==AssignOwn)store_->resetFailoverPrioritySequence();

  endEspNow();
  WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.persistent(false);
  WifiCredential received{ssid,password};
  if(!connectOne(received,12000)){error=config_->language=="en"?"Provisioned Wi-Fi could not be connected":"Das übernommene WLAN konnte nicht verbunden werden";return false;}
  setupMode_=false;provisioningWait_=false;
  return true;
}

bool WifiManager::tryEspNowProvisioning() {
  if(!store_||!config_)return false;
  WiFi.disconnect(false,false);WiFi.mode(WIFI_STA);WiFi.persistent(false);delay(80);
  if(!beginEspNow())return false;
  provisioningWait_=true;
  const uint32_t provisioningStarted=millis();
  bool sawMaster=false;unsigned emptySweeps=0;
  Serial.println("ESP-NOW: Suche nach vorhandenem AntennaController-Master ...");

  for(;;){
    if(millis()-provisioningStarted>=20000){provisioningWait_=false;endEspNow();Serial.println("ESP-NOW: Zeitlimit erreicht, Setup-AP wird verwendet");return false;}
    offerCount_=0;credentialReady_=false;credentialLength_=0;activeProvisionNonce_=esp_random();
    for(uint8_t channel=1;channel<=13;++channel){
      sendProvisionProbe(channel,activeProvisionNonce_);
      uint32_t wait=millis();bool repeated=false;
      while(millis()-wait<170){delay(10);if(!repeated&&millis()-wait>70){sendProvisionProbe(channel,activeProvisionNonce_);repeated=true;}}
    }
    size_t count=offerCount_;if(count>8)count=8;
    if(count==0){
      if(!sawMaster&&++emptySweeps>=2){provisioningWait_=false;endEspNow();Serial.println("ESP-NOW: kein Master gefunden, Setup-AP wird verwendet");return false;}
      delay(180);continue;
    }
    sawMaster=true;emptySweeps=0;

    int selected=-1;uint8_t assignment=AssignNone;int authorizedCount=0;
    for(size_t i=0;i<count;++i)if(offers_[i].assignment!=AssignNone){authorizedCount++;selected=(int)i;assignment=offers_[i].assignment;}
    if(authorizedCount!=1){selected=-1;assignment=AssignNone;}
    if(authorizedCount==0&&count==1&&offers_[0].admission==AdmissionAuto&&config_->federation.role=="unassigned"&&config_->federation.systemId.isEmpty()){selected=0;assignment=AssignJoin;}

    if(selected>=0){
      OfferRecord offer=offers_[selected];activeProvisionNonce_=offer.nonce;
      if(requestProvisioning(offer,assignment)){
        String e;if(applyReceivedCredentials(e)){Serial.println("ESP-NOW: WLAN und Anlagenrolle automatisch übernommen");beginEspNow();return true;}
        if(store_)store_->addError("ESPNOW_APPLY",e);
      }
    }
    Serial.println(count>1?"ESP-NOW: mehrere Anlagen gefunden - warte auf Auswahl am Master":"ESP-NOW: Master gefunden - warte auf Freigabe am Master");
    delay(250);
  }
}

void WifiManager::loop() {
  processEspNow();
  processDns();
  if (setupMode_) { serviceFallbackReconnect(); return; }

  const bool pressed = digitalRead(BootButtonGpio) == LOW;
  if (pressed) {
    if (!bootPressStart_) bootPressStart_ = millis();
    if (!bootPressLatched_ && millis() - bootPressStart_ >= BootHoldMs) {
      bootPressLatched_ = true;
      requestSetupMode();
      delay(150);
      ESP.restart();
    }
  } else {
    bootPressStart_ = 0;
    bootPressLatched_ = false;
  }
}

void WifiManager::serviceFallbackReconnect() {
  if (!fallbackRecoveryMode_ || !store_) return;
  const auto nets = store_->wifiNetworks();
  if (nets.empty()) return;
  const uint32_t now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!fallbackConnecting_) return;
    if (fallbackConnectedAt_ == 0) {
      fallbackConnectedAt_ = now;
      Serial.println("WLAN: Fallback-Wiederverbindung erfolgreich; uebernehme normalen Start ...");
      return;
    }
    if (now - fallbackConnectedAt_ >= 1000) ESP.restart();
    return;
  }
  fallbackConnectedAt_ = 0;

  if (fallbackConnecting_) {
    if (now - fallbackConnectStartedAt_ < ConnectTimeoutMs) return;
    WiFi.disconnect(false, false);
    fallbackConnecting_ = false;
    fallbackNetworkIndex_ = (fallbackNetworkIndex_ + 1) % nets.size();
    fallbackNextTryAt_ = now + 5000;
    Serial.println("WLAN: Fallback-Wiederverbindung fehlgeschlagen; naechster gespeicherter Zugang folgt.");
    return;
  }
  if ((int32_t)(now - fallbackNextTryAt_) < 0) return;

  const WifiCredential& net = nets[fallbackNetworkIndex_ % nets.size()];
  WiFi.mode(WIFI_AP_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.begin(net.ssid.c_str(), net.password.c_str());
  fallbackConnecting_ = true;
  fallbackConnectStartedAt_ = now;
  Serial.println("WLAN: Fallback-Wiederverbindung startet fuer gespeicherten Zugang " + String(fallbackNetworkIndex_ + 1));
}

