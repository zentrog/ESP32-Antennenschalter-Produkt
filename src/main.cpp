#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "Model.h"
#include "Storage.h"
#include "RelayEngine.h"
#include "Services.h"
#include "WifiManager.h"
#include "WebUi.h"

LocalConfig localCfg;
SharedConfig sharedCfg;
Storage storage;
RelayEngine relays;
TimeService clockSvc;
MqttService mqtt;
FederationService federation;
NewsService news;
WeatherService weather;
WifiManager wifiManager;
WebUi web;
bool centralServicesActive=false;
String mdnsHostActive;
bool mdnsRunning=false;

static String desiredMdnsHost(){
  // Ein permanenter Master kennt seinen stabilen Anlagenhost bereits beim Boot.
  // Ein Follower verwendet den Anlagenhost nur waehrend einer echten temporaeren Koordinator-Uebernahme.
  if(federation.isMaster()||federation.isTemporaryCoordinator()){String h=federation.systemHost();if(!h.isEmpty())return h;}
  return localCfg.identity.hostName.isEmpty()?String("antenna-controller"):localCfg.identity.hostName;
}

static void refreshMdnsResponder(bool force=false){
  if(wifiManager.setupMode()||WiFi.status()!=WL_CONNECTED){if(mdnsRunning){MDNS.end();mdnsRunning=false;mdnsHostActive="";}return;}
  String wanted=desiredMdnsHost();
  if(!force&&mdnsRunning&&wanted==mdnsHostActive)return;
  if(mdnsRunning)MDNS.end();
  mdnsRunning=MDNS.begin(wanted.c_str());
  mdnsHostActive=mdnsRunning?wanted:String();
  if(mdnsRunning){
    MDNS.addService("http","tcp",80);
    MDNS.addServiceTxt("http","tcp","role",federation.isCoordinator()?"coordinator":localCfg.federation.role);
    MDNS.addServiceTxt("http","tcp","device",localCfg.identity.deviceName);
    Serial.println("mDNS: http://"+wanted+".local/");
  }else Serial.println("mDNS konnte nicht gestartet werden: "+wanted);
}

void setup(){
  Serial.begin(115200);
  delay(100);

  if(!storage.begin()){
    Serial.println("LittleFS/NVS Fehler");
    return;
  }

  String e;
  storage.loadLocal(localCfg,e);
  storage.loadShared(sharedCfg,e);

  // Fresh-distribution example: shared defaults are created independently from local defaults,
  // therefore bind the example layout/route to the real local controller ID once both are loaded.
  bool examplePresent=false;for(const auto&r:sharedCfg.routes)if(r.id=="example-route")examplePresent=true;
  if(examplePresent){String prefix=localCfg.controllerId+":";bool changed=false;for(auto&li:sharedCfg.layout){int p=li.key.indexOf(':');String id=p>=0?li.key.substring(p+1):li.key;if(id=="example-radio"||id=="example-pa"||id=="example-ant"){String wanted=prefix+id;if(li.key!=wanted){li.key=wanted;changed=true;}}}for(auto&r:sharedCfg.routes)if(r.id=="example-route"){std::vector<String>wanted={prefix+"example-radio",prefix+"example-pa",prefix+"example-ant"};if(r.deviceKeys!=wanted){r.deviceKeys=wanted;changed=true;}}if(changed){sharedCfg.revision++;sharedCfg.revisionOrigin=localCfg.controllerId;String xe;if(!storage.saveShared(sharedCfg,xe))storage.addError("EXAMPLE_LAYOUT",xe);}}

  // v1.7 migration: administrator protection belongs to the whole installation, not to one ESP.
  if(localCfg.security.adminAuthEnabled && !sharedCfg.security.adminAuthEnabled){
    sharedCfg.security=localCfg.security;sharedCfg.schema=5;sharedCfg.revision++;sharedCfg.revisionOrigin=localCfg.controllerId;
    String se;if(storage.saveShared(sharedCfg,se)){localCfg.security.adminAuthEnabled=false;String le;storage.saveLocal(localCfg,le);}else storage.addError("SECURITY_MIGRATION",se);
  }

  relays.begin(&localCfg,&storage);
  relays.safeInit();
  // Each controller restores its own persisted antenna/RF selections on boot.
  // This lets a master and its followers recover the last route after a power loss.
  relays.restore(true);
  if(sharedCfg.systemStormMode)relays.setStormMode(true);

  // Local saved routes are restored before federation starts; each node restores only its own outputs.
  wifiManager.begin(&storage,&localCfg);

  if(!wifiManager.setupMode()){
    clockSvc.begin(localCfg.time);
    mqtt.begin(&localCfg,&relays);
    federation.begin(&localCfg,&sharedCfg,&storage,&relays);
    news.begin(&localCfg);
    weather.begin(&localCfg,&storage);
    centralServicesActive=federation.isCoordinator();
  }

  web.begin(&localCfg,&sharedCfg,&storage,&relays,&clockSvc,&mqtt,&federation,&news,&weather,&wifiManager,wifiManager.setupMode());
}

void loop(){
  relays.loop();
  wifiManager.loop();
  web.loop();

  if(!wifiManager.setupMode()){
    federation.loop();
    refreshMdnsResponder();
    bool shouldRunCentral=federation.isCoordinator();
    if(shouldRunCentral!=centralServicesActive){centralServicesActive=shouldRunCentral;mqtt.configChanged();news.configChanged();weather.configChanged();}
    if(shouldRunCentral){mqtt.loop();news.loop();weather.loop();}
  }
  delay(2);
}
