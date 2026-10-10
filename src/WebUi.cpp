#include "WebUi.h"
#include "BoardProfiles.h"
#include "Version.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "GitHubTrustBundle.h"
#include <Update.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <esp_ota_ops.h>
#include <sys/stat.h>
#include <cstring>
#include <vector>

static const char* PREUPDATE_LOCAL="/preupdate-local.json";
static const char* PREUPDATE_SHARED="/preupdate-shared.json";
static const char* PREUPDATE_WIFI="/preupdate-wifi.json";
static const size_t LITTLEFS_SAFETY_RESERVE=256U*1024U;
static bool uiUploadOk=false;
static String uiUploadError,uiUploadTarget,uiUploadTmp,uiUploadBackup;
static File uiUploadFile;
static uint32_t safetyBackupFreshAtMs=0;
static bool updateBackupDownloaded=false;
static const uint32_t SAFETY_BACKUP_FRESH_MS=5U*60U*1000U;

static String safeBackupPart(String value){
  String out;out.reserve(value.length());
  for(size_t i=0;i<value.length();++i){char c=value[i];if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_')out+=c;else if(c=='.')out+='_';}
  return out.isEmpty()?String("unknown"):out;
}

#pragma pack(push,1)
struct UiBundleFooter {
  char magic[8];
  uint32_t indexBrOffset,indexBrSize,appBrOffset,appBrSize,responsiveCssBrOffset,responsiveCssBrSize,styleCssBrOffset,styleCssBrSize;
  uint32_t indexGzipOffset,indexGzipSize,appGzipOffset,appGzipSize,responsiveCssGzipOffset,responsiveCssGzipSize,styleCssGzipOffset,styleCssGzipSize;
  uint32_t payloadCrc32;
};
#pragma pack(pop)
static_assert(sizeof(UiBundleFooter)==76,"OTA UI bundle footer layout must match the package builder");
static const char UI_BUNDLE_MAGIC[8]={'A','N','T','U','I','B','R','3'};
static const esp_partition_t* uiBundlePartition=nullptr;
static UiBundleFooter uiBundleFooter{};
static bool uiBundleChecked=false,uiBundleValid=false;
static String acceptedUiEncoding(WebServer&server){
  String enc=server.header("Accept-Encoding");enc.toLowerCase();int start=0;
  bool br=false,gzip=false;
  while(start<(int)enc.length()){
    int end=enc.indexOf(',',start);if(end<0)end=enc.length();String item=enc.substring(start,end);item.trim();
    int semi=item.indexOf(';');String coding=semi<0?item:item.substring(0,semi);coding.trim();
    bool enabled=true;int q=item.indexOf("q=",semi<0?0:semi+1);
    if(q>=0){int qEnd=item.indexOf(';',q);String quality=item.substring(q+2,qEnd<0?item.length():qEnd);quality.trim();enabled=quality.toFloat()>0.0f;}
    if(enabled&&coding=="br")br=true;else if(enabled&&coding=="gzip")gzip=true;
    start=end+1;
  }
  // The ESP web UI is currently plain HTTP. Prefer gzip for broad browser compatibility;
  // a browser that only supports Brotli still receives the Brotli representation.
  return gzip?String("gzip"):br?String("br"):String();
}
static uint32_t uiBundleCrc32(uint32_t crc,const uint8_t*data,size_t n){
  crc=~crc;for(size_t i=0;i<n;++i){crc^=data[i];for(uint8_t b=0;b<8;++b)crc=(crc>>1)^(0xEDB88320U&-(crc&1U));}return ~crc;
}
static bool loadUiBundle(){
  if(uiBundleChecked)return uiBundleValid;uiBundleChecked=true;
  uiBundlePartition=esp_ota_get_running_partition();if(!uiBundlePartition||uiBundlePartition->size<sizeof(UiBundleFooter))return false;
  uint32_t footerAt=uiBundlePartition->size-sizeof(UiBundleFooter);
  if(esp_partition_read(uiBundlePartition,footerAt,&uiBundleFooter,sizeof(uiBundleFooter))!=ESP_OK)return false;
  if(memcmp(uiBundleFooter.magic,UI_BUNDLE_MAGIC,sizeof(UI_BUNDLE_MAGIC))!=0)return false;
  uint64_t indexBrEnd=(uint64_t)uiBundleFooter.indexBrOffset+uiBundleFooter.indexBrSize;
  uint64_t appBrEnd=(uint64_t)uiBundleFooter.appBrOffset+uiBundleFooter.appBrSize;
  uint64_t responsiveCssBrEnd=(uint64_t)uiBundleFooter.responsiveCssBrOffset+uiBundleFooter.responsiveCssBrSize;
  uint64_t styleCssBrEnd=(uint64_t)uiBundleFooter.styleCssBrOffset+uiBundleFooter.styleCssBrSize;
  uint64_t indexGzipEnd=(uint64_t)uiBundleFooter.indexGzipOffset+uiBundleFooter.indexGzipSize;
  uint64_t appGzipEnd=(uint64_t)uiBundleFooter.appGzipOffset+uiBundleFooter.appGzipSize;
  uint64_t responsiveCssGzipEnd=(uint64_t)uiBundleFooter.responsiveCssGzipOffset+uiBundleFooter.responsiveCssGzipSize;
  uint64_t styleCssGzipEnd=(uint64_t)uiBundleFooter.styleCssGzipOffset+uiBundleFooter.styleCssGzipSize;
  if(uiBundleFooter.indexBrOffset<0x1000||uiBundleFooter.indexBrSize==0||uiBundleFooter.appBrSize==0||uiBundleFooter.responsiveCssBrSize==0||uiBundleFooter.styleCssBrSize==0||
     uiBundleFooter.indexGzipSize==0||uiBundleFooter.appGzipSize==0||uiBundleFooter.responsiveCssGzipSize==0||uiBundleFooter.styleCssGzipSize==0||
     indexBrEnd!=uiBundleFooter.appBrOffset||appBrEnd!=uiBundleFooter.responsiveCssBrOffset||responsiveCssBrEnd!=uiBundleFooter.styleCssBrOffset||styleCssBrEnd!=uiBundleFooter.indexGzipOffset||
     indexGzipEnd!=uiBundleFooter.appGzipOffset||appGzipEnd!=uiBundleFooter.responsiveCssGzipOffset||responsiveCssGzipEnd!=uiBundleFooter.styleCssGzipOffset||styleCssGzipEnd>footerAt)return false;
  uint32_t crc=0;uint8_t buf[512];uint32_t left=(uint32_t)(styleCssGzipEnd-uiBundleFooter.indexBrOffset),at=uiBundleFooter.indexBrOffset;
  while(left){size_t n=left>sizeof(buf)?sizeof(buf):left;if(esp_partition_read(uiBundlePartition,at,buf,n)!=ESP_OK)return false;crc=uiBundleCrc32(crc,buf,n);at+=(uint32_t)n;left-=(uint32_t)n;}
  uiBundleValid=crc==uiBundleFooter.payloadCrc32;return uiBundleValid;
}
static bool sendBundledUi(WebServer&server,const char*name,const char*mime){
  server.sendHeader("Vary","Accept-Encoding");
  String encoding=acceptedUiEncoding(server);uint32_t offset=0,length=0;
  if(encoding=="br"){
    if(strcmp(name,"index.html")==0){offset=uiBundleFooter.indexBrOffset;length=uiBundleFooter.indexBrSize;}
    else if(strcmp(name,"app.js")==0){offset=uiBundleFooter.appBrOffset;length=uiBundleFooter.appBrSize;}
    else if(strcmp(name,"responsive.css")==0){offset=uiBundleFooter.responsiveCssBrOffset;length=uiBundleFooter.responsiveCssBrSize;}
    else{offset=uiBundleFooter.styleCssBrOffset;length=uiBundleFooter.styleCssBrSize;}
  }else if(encoding=="gzip"){
    if(strcmp(name,"index.html")==0){offset=uiBundleFooter.indexGzipOffset;length=uiBundleFooter.indexGzipSize;}
    else if(strcmp(name,"app.js")==0){offset=uiBundleFooter.appGzipOffset;length=uiBundleFooter.appGzipSize;}
    else if(strcmp(name,"responsive.css")==0){offset=uiBundleFooter.responsiveCssGzipOffset;length=uiBundleFooter.responsiveCssGzipSize;}
    else{offset=uiBundleFooter.styleCssGzipOffset;length=uiBundleFooter.styleCssGzipSize;}
  }else return false;
  if(length==0)return false;
  server.sendHeader("Cache-Control",strcmp(name,"index.html")==0?"no-cache, must-revalidate":"public, max-age=31536000, immutable");
  server.sendHeader("Content-Encoding",encoding);server.setContentLength(length);server.send(200,mime,"");
  uint8_t buf[512];uint32_t sent=0;while(sent<length){size_t n=length-sent>sizeof(buf)?sizeof(buf):length-sent;if(esp_partition_read(uiBundlePartition,offset+sent,buf,n)!=ESP_OK){server.client().stop();return true;}server.sendContent((const char*)buf,n);sent+=(uint32_t)n;}return true;
}
static void sendUiEncodingError(WebServer&server){
  server.sendHeader("Cache-Control","no-store");
  server.send(406,"text/plain; charset=utf-8","Dieser Browser fordert weder Brotli noch gzip an. Bitte einen aktuellen Browser verwenden und die Seite neu laden.");
}

static bool littleFsExistsQuiet(const char* path){
  if(!path||!*path)return false;String full="/littlefs"+String(path);struct stat st;return ::stat(full.c_str(),&st)==0;
}
static void removeBundledUiFromLittleFs(){
  if(!loadUiBundle())return;
  const char* obsolete[]={"/index.html","/app.js","/responsive.css","/style.css"};
  for(const char* path:obsolete)if(littleFsExistsQuiet(path)&&!LittleFS.remove(path))Serial.printf("Could not remove obsolete UI file %s\n",path);
}
static bool removeIfExists(const char* path){
  return !littleFsExistsQuiet(path)||LittleFS.remove(path);
}
static bool removeIfExists(const String& path){
  return removeIfExists(path.c_str());
}

static String controllerDisplayName(const String&base,const String&role,uint16_t priority){
  String n=base.isEmpty()?String("Antennencontroller"):base;
  if(role=="master")return n+"-0";
  if(role=="follower"){uint16_t suffix=priority<100?1:(uint16_t)(1+(priority-100)/10);return n+"-"+String(suffix);}
  return n;
}
static int compareReleaseVersion(String a,String b){
  a.trim();b.trim();if(a.startsWith("v"))a.remove(0,1);if(b.startsWith("v"))b.remove(0,1);
  for(int part=0;part<4;++part){int da=a.indexOf('.'),db=b.indexOf('.');String sa=da<0?a:a.substring(0,da),sb=db<0?b:b.substring(0,db);int va=sa.toInt(),vb=sb.toInt();if(va<vb)return-1;if(va>vb)return 1;if(da<0&&db<0)break;a=da<0?String():a.substring(da+1);b=db<0?String():b.substring(db+1);}return 0;
}
static void sendOnlineUpdateCheck(WebServer&server,const LocalConfig*c){
  JsonDocument out;out["current"]=ANTCTRL_VERSION;out["source"]="https://api.github.com/repos/zentrog/ESP32-Antennenschalter-Produkt/releases/latest";out["downloadPage"]="https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/latest";
  if(WiFi.status()!=WL_CONNECTED){out["error"]=c&&c->language=="en"?"No internet connection":"Keine Internetverbindung";String raw;serializeJson(out,raw);server.send(503,"application/json; charset=utf-8",raw);return;}
  WiFiClientSecure tls;tls.setInsecure();HTTPClient h;h.setTimeout(6000);
  if(!h.begin(tls,"https://api.github.com/repos/zentrog/ESP32-Antennenschalter-Produkt/releases/latest")){out["error"]=c&&c->language=="en"?"GitHub could not be reached":"GitHub konnte nicht erreicht werden";String raw;serializeJson(out,raw);server.send(502,"application/json; charset=utf-8",raw);return;}
  h.addHeader("User-Agent","AntennaController/"+String(ANTCTRL_VERSION));h.addHeader("Accept","application/vnd.github+json");
  int code=h.GET();String body=code==200?h.getString():String();h.end();if(code!=200){out["httpCode"]=code;out["error"]=(c&&c->language=="en"?"Update server HTTP ":"Update-Server HTTP ")+String(code);String raw;serializeJson(out,raw);server.send(502,"application/json; charset=utf-8",raw);return;}
  JsonDocument d;if(deserializeJson(d,body)){out["error"]=c&&c->language=="en"?"Invalid GitHub release information":"Ungültige GitHub-Release-Information";String raw;serializeJson(out,raw);server.send(502,"application/json; charset=utf-8",raw);return;}
  String latest=String((const char*)(d["tag_name"]|""));bool stable=!(d["prerelease"]|true);if(latest.isEmpty()||!stable){out["error"]=c&&c->language=="en"?"No stable GitHub release published":"Kein stabiles GitHub-Release veröffentlicht";String raw;serializeJson(out,raw);server.send(200,"application/json; charset=utf-8",raw);return;}
  out["latest"]=latest;out["channel"]="stable";out["available"]=compareReleaseVersion(String(ANTCTRL_VERSION),latest)<0;out["notes"]=String((const char*)(d["body"]|""));if(d["html_url"].is<const char*>())out["downloadPage"]=String((const char*)d["html_url"]);String raw;serializeJson(out,raw);server.send(200,"application/json; charset=utf-8",raw);
}

static bool readJsonChecked(const char* path,String& raw,String& error){
  File f=LittleFS.open(path,"r");if(!f){error="Datei fehlt: "+String(path);return false;}
  raw=f.readString();f.close();if(raw.isEmpty()){error="Datei ist leer: "+String(path);return false;}
  JsonDocument d;DeserializationError de=deserializeJson(d,raw);if(de){error="Ungültiges JSON in "+String(path)+": "+String(de.c_str());return false;}return true;
}
static bool writeJsonChecked(const char* path,const String& raw,String& error){
  File f=LittleFS.open(path,"w");if(!f){error="Sicherung kann nicht geschrieben werden: "+String(path);return false;}
  size_t n=f.print(raw);f.flush();f.close();if(n!=raw.length()){LittleFS.remove(path);error="Sicherung wurde unvollständig geschrieben: "+String(path);return false;}
  String check;if(!readJsonChecked(path,check,error)){LittleFS.remove(path);return false;}if(check.length()!=raw.length()){LittleFS.remove(path);error="Sicherungsprüfung meldet falsche Dateigröße: "+String(path);return false;}return true;
}
static bool createUpdateSafetyBackup(Storage* store,String& error){
  if(!store){error="Speicherverwaltung nicht verfügbar";return false;}
  String local,shared;if(!readJsonChecked("/local.json",local,error))return false;if(!readJsonChecked("/shared.json",shared,error))return false;
  JsonDocument w;w["format"]="AntennaControllerWifiBackupV1";auto a=w["wifi"].to<JsonArray>();for(const auto&n:store->wifiNetworks()){auto o=a.add<JsonObject>();o["ssid"]=n.ssid;o["password"]=n.password;}String wifi;serializeJson(w,wifi);
  const char* tl="/preupdate-local.tmp";const char* ts="/preupdate-shared.tmp";const char* tw="/preupdate-wifi.tmp";removeIfExists(tl);removeIfExists(ts);removeIfExists(tw);
  size_t freeBytes=LittleFS.totalBytes()>LittleFS.usedBytes()?LittleFS.totalBytes()-LittleFS.usedBytes():0;size_t need=local.length()+shared.length()+wifi.length();
  if(freeBytes<need+LITTLEFS_SAFETY_RESERVE){error="Zu wenig freier Datenspeicher für sichere Aktualisierung. Die vorhandene Sicherung bleibt erhalten; benötigt werden neue Sicherung plus 256 KiB Reserve.";return false;}
  if(!writeJsonChecked(tl,local,error)){LittleFS.remove(tl);return false;}if(!writeJsonChecked(ts,shared,error)){LittleFS.remove(tl);LittleFS.remove(ts);return false;}if(!writeJsonChecked(tw,wifi,error)){LittleFS.remove(tl);LittleFS.remove(ts);LittleFS.remove(tw);return false;}
  removeIfExists(PREUPDATE_LOCAL);removeIfExists(PREUPDATE_SHARED);removeIfExists(PREUPDATE_WIFI);if(!LittleFS.rename(tl,PREUPDATE_LOCAL)||!LittleFS.rename(ts,PREUPDATE_SHARED)||!LittleFS.rename(tw,PREUPDATE_WIFI)){removeIfExists(tl);removeIfExists(ts);removeIfExists(tw);error="Geprüfte Sicherheitskopie konnte nicht aktiviert werden; Aktualisierung wird verweigert";return false;}safetyBackupFreshAtMs=millis();return true;
}
static bool updateSafetyBackupFresh(){
  if(!safetyBackupFreshAtMs)return false;
  if((uint32_t)(millis()-safetyBackupFreshAtMs)>SAFETY_BACKUP_FRESH_MS)return false;
  return littleFsExistsQuiet(PREUPDATE_LOCAL)&&littleFsExistsQuiet(PREUPDATE_SHARED)&&littleFsExistsQuiet(PREUPDATE_WIFI);
}
static bool ensureFreshUpdateSafetyBackup(Storage* store,String& error){
  return updateSafetyBackupFresh()?true:createUpdateSafetyBackup(store,error);
}
static void sendUpdateSafetyBackup(WebServer& server,Storage* store,const LocalConfig* config,const String& firmware){
  String error;if(!createUpdateSafetyBackup(store,error)){JsonDocument r;r["ok"]=false;r["error"]=error;String out;serializeJson(r,out);server.send(409,"application/json; charset=utf-8",out);return;}
  String local,shared,wifi;if(!readJsonChecked(PREUPDATE_LOCAL,local,error)||!readJsonChecked(PREUPDATE_SHARED,shared,error)||!readJsonChecked(PREUPDATE_WIFI,wifi,error)){server.send(500,"text/plain; charset=utf-8",error);return;}
  String controllerId=safeBackupPart(config?config->controllerId:String()),ip=safeBackupPart(WiFi.localIP().toString());
  String filename="AntennaController-backup-"+controllerId+"-"+ip+".json";
  server.sendHeader("Content-Disposition","attachment; filename=\""+filename+"\"");server.setContentLength(CONTENT_LENGTH_UNKNOWN);server.send(200,"application/json; charset=utf-8","");
  JsonDocument metadata;JsonObject ci=metadata["controllerInfo"].to<JsonObject>();if(config){ci["controllerId"]=config->controllerId;ci["name"]=config->identity.deviceName;ci["role"]=config->federation.role;}ci["ip"]=WiFi.localIP().toString();String metadataJson;serializeJson(metadata,metadataJson);
  server.sendContent("{\"format\":\"AntennaControllerSafetyBackupV2\",\"firmware\":\"");server.sendContent(firmware);server.sendContent("\",");server.sendContent(metadataJson.substring(1,metadataJson.length()-1));server.sendContent(",\"local\":");server.sendContent(local);server.sendContent(",\"shared\":");server.sendContent(shared);server.sendContent(",\"wifiState\":");server.sendContent(wifi);server.sendContent("}");updateBackupDownloaded=true;
}
static String cleanUploadName(String name){name.replace("\\","/");int p=name.lastIndexOf('/');if(p>=0)name=name.substring(p+1);return name;}
static bool allowedUiFile(const String& n){return n=="app.js"||n=="style.css"||n=="index.html"||n=="setup.html"||n=="logo.png"||n=="favicon.ico";}
static void handleUiUpload(HTTPUpload& u,Storage* store){
  if(u.status==UPLOAD_FILE_START){
    uiUploadOk=false;uiUploadError="";uiUploadTarget="";if(uiUploadFile)uiUploadFile.close();if(loadUiBundle()){uiUploadError="Die Weboberfläche wird vollständig mit der Firmware aktualisiert. Einzeldateien werden abgewiesen, damit keine alte Oberfläche mit dem aktuellen Paket vermischt wird.";return;}String backupError;if(!ensureFreshUpdateSafetyBackup(store,backupError)){uiUploadError="Sicherheitskopie fehlgeschlagen. UI-Aktualisierung verweigert: "+backupError;return;}String name=cleanUploadName(u.filename);if(!allowedUiFile(name)){uiUploadError="Nur freigegebene Weboberflächen-Dateien dürfen aktualisiert werden";return;}
    size_t freeBytes=LittleFS.totalBytes()>LittleFS.usedBytes()?LittleFS.totalBytes()-LittleFS.usedBytes():0;if(freeBytes<LITTLEFS_SAFETY_RESERVE+64U*1024U){uiUploadError="Zu wenig freier Datenspeicher; 256 KiB Sicherheitsreserve müssen erhalten bleiben";return;}
    uiUploadTarget="/"+name;uiUploadTmp=uiUploadTarget+".upload";uiUploadBackup=uiUploadTarget+".old";removeIfExists(uiUploadTmp);uiUploadFile=LittleFS.open(uiUploadTmp.c_str(),"w");if(!uiUploadFile){uiUploadError="Temporäre UI-Datei kann nicht angelegt werden";return;}uiUploadOk=true;
  }else if(u.status==UPLOAD_FILE_WRITE){
    if(!uiUploadOk)return;size_t freeBytes=LittleFS.totalBytes()>LittleFS.usedBytes()?LittleFS.totalBytes()-LittleFS.usedBytes():0;if(freeBytes<u.currentSize+LITTLEFS_SAFETY_RESERVE){uiUploadOk=false;uiUploadError="UI-Aktualisierung würde die 256-KiB-Datenreserve unterschreiten";uiUploadFile.close();removeIfExists(uiUploadTmp);return;}if(uiUploadFile.write(u.buf,u.currentSize)!=u.currentSize){uiUploadOk=false;uiUploadError="UI-Datei konnte nicht vollständig geschrieben werden";uiUploadFile.close();removeIfExists(uiUploadTmp);}
  }else if(u.status==UPLOAD_FILE_END){
    if(uiUploadFile)uiUploadFile.close();if(!uiUploadOk){removeIfExists(uiUploadTmp);return;}File check=LittleFS.open(uiUploadTmp.c_str(),"r");size_t sz=check?check.size():0;if(check)check.close();if(sz==0){uiUploadOk=false;uiUploadError="Hochgeladene UI-Datei ist leer";removeIfExists(uiUploadTmp);return;}
    removeIfExists(uiUploadBackup);bool hadOld=littleFsExistsQuiet(uiUploadTarget.c_str());if(hadOld&&!LittleFS.rename(uiUploadTarget.c_str(),uiUploadBackup.c_str())){uiUploadOk=false;uiUploadError="Vorhandene UI-Datei konnte nicht gesichert werden";removeIfExists(uiUploadTmp);return;}if(!LittleFS.rename(uiUploadTmp.c_str(),uiUploadTarget.c_str())){if(hadOld)LittleFS.rename(uiUploadBackup.c_str(),uiUploadTarget.c_str());uiUploadOk=false;uiUploadError="Neue UI-Datei konnte nicht aktiviert werden";removeIfExists(uiUploadTmp);return;}removeIfExists(uiUploadBackup);uiUploadOk=true;
  }else if(u.status==UPLOAD_FILE_ABORTED){if(uiUploadFile)uiUploadFile.close();removeIfExists(uiUploadTmp);uiUploadOk=false;uiUploadError="UI-Upload wurde abgebrochen";}
}


static void customBoardJson(JsonObject o,const CustomBoardConfig& b){
  o["name"]=b.name;o["note"]=b.note;auto a=o["pins"].to<JsonArray>();
  for(const auto& p:b.pins){auto q=a.add<JsonObject>();q["label"]=p.label;q["gpio"]=p.gpio;q["kind"]=p.kind;q["digitalInput"]=p.digitalInput;q["digitalOutput"]=p.digitalOutput;q["analogInput"]=p.analogInput;q["adcUnit"]=p.adcUnit;q["pullUp"]=p.pullUp;q["pullDown"]=p.pullDown;q["strapping"]=p.strapping;q["reserved"]=p.reserved;q["side"]=p.side;q["order"]=p.order;q["note"]=p.note;}
}
static void customBoardFromJson(JsonVariantConst q,CustomBoardConfig& b){
  if(q.isNull())return;b.name=String((const char*)(q["name"]|b.name.c_str()));b.note=String((const char*)(q["note"]|b.note.c_str()));b.pins.clear();
  for(JsonObjectConst o:q["pins"].as<JsonArrayConst>()){CustomBoardPinConfig p;p.label=String((const char*)(o["label"]|""));p.gpio=o["gpio"]|-1;p.kind=String((const char*)(o["kind"]|"gpio"));p.digitalInput=o["digitalInput"]|true;p.digitalOutput=o["digitalOutput"]|false;p.analogInput=o["analogInput"]|false;p.adcUnit=o["adcUnit"]|0;p.pullUp=o["pullUp"]|false;p.pullDown=o["pullDown"]|false;p.strapping=o["strapping"]|false;p.reserved=o["reserved"]|false;p.side=String((const char*)(o["side"]|"left"));p.order=o["order"]|0;p.note=String((const char*)(o["note"]|""));b.pins.push_back(p);}
}
static bool sameCustomBoard(const CustomBoardConfig&a,const CustomBoardConfig&b){
  if(a.name!=b.name||a.note!=b.note||a.pins.size()!=b.pins.size())return false;
  for(size_t i=0;i<a.pins.size();++i){const auto&x=a.pins[i];const auto&y=b.pins[i];if(x.label!=y.label||x.gpio!=y.gpio||x.kind!=y.kind||x.digitalInput!=y.digitalInput||x.digitalOutput!=y.digitalOutput||x.analogInput!=y.analogInput||x.adcUnit!=y.adcUnit||x.pullUp!=y.pullUp||x.pullDown!=y.pullDown||x.strapping!=y.strapping||x.reserved!=y.reserved||x.side!=y.side||x.order!=y.order||x.note!=y.note)return false;}
  return true;
}


static void jsonStringVector(JsonArray a,const std::vector<String>& values){for(const auto&v:values)a.add(v);}
static void jsonStringVectorFrom(JsonVariantConst q,std::vector<String>& values){values.clear();for(JsonVariantConst v:q.as<JsonArrayConst>()){String x=String((const char*)(v|""));if(!x.isEmpty())values.push_back(x);}}
static void deviceJson(JsonObject o,const LogicalDeviceConfig& d){o["id"]=d.id;o["name"]=d.name;o["category"]=d.category;o["displayGroupId"]=d.displayGroupId;o["enabled"]=d.enabled;o["exclusive"]=d.exclusive;o["stormRelayId"]=d.stormRelayId;o["stormRelayOn"]=d.stormRelayOn;o["note"]=d.note;jsonStringVector(o["bands"].to<JsonArray>(),d.bands);jsonStringVector(o["functionIds"].to<JsonArray>(),d.functionIds);}
static void deviceFromJson(JsonObjectConst q,LogicalDeviceConfig& d){d.id=String((const char*)(q["id"]|""));d.name=String((const char*)(q["name"]|""));d.category=String((const char*)(q["category"]|"other"));d.displayGroupId=String((const char*)(q["displayGroupId"]|""));d.enabled=q["enabled"]|true;d.exclusive=q["exclusive"]|false;d.note=String((const char*)(q["note"]|""));jsonStringVectorFrom(q["bands"],d.bands);jsonStringVectorFrom(q["functionIds"],d.functionIds);}
static void displayGroupJson(JsonObject o,const DisplayGroupConfig& g){o["id"]=g.id;o["title"]=g.title;o["order"]=g.order;}
static void displayGroupFromJson(JsonObjectConst q,DisplayGroupConfig& g){g.id=String((const char*)(q["id"]|""));g.title=String((const char*)(q["title"]|""));g.order=q["order"]|0;}
static void routeJson(JsonObject o,const RouteConfig& r){o["id"]=r.id;o["label"]=r.label;o["color"]=r.color;o["enabled"]=r.enabled;o["bandOverride"]=r.bandOverride;o["overrideNote"]=r.overrideNote;jsonStringVector(o["deviceKeys"].to<JsonArray>(),r.deviceKeys);}
static void routeFromJson(JsonObjectConst q,RouteConfig& r){r.id=String((const char*)(q["id"]|""));r.label=String((const char*)(q["label"]|""));r.color=String((const char*)(q["color"]|"#3f8fd2"));r.enabled=q["enabled"]|true;r.bandOverride=q["bandOverride"]|false;r.overrideNote=String((const char*)(q["overrideNote"]|""));jsonStringVectorFrom(q["deviceKeys"],r.deviceKeys);}

static void uiJson(JsonObject o,const UiDefaults&v){o["buttonWidth"]=v.buttonWidth;o["buttonHeight"]=v.buttonHeight;o["defaultCols"]=v.defaultCols;o["defaultRows"]=v.defaultRows;o["fontPx"]=v.fontPx;o["normalColor"]=v.normalColor;o["activeColor"]=v.activeColor;o["powerInactiveColor"]=v.powerInactiveColor;o["powerActiveColor"]=v.powerActiveColor;o["lockedColor"]=v.lockedColor;o["rememberedColor"]=v.rememberedColor;o["runningColor"]=v.runningColor;}
static void cfgJson(JsonDocument&d,const LocalConfig&c,bool includeSecrets){
 d["schema"]=c.schema;d["language"]=c.language;d["revision"]=c.revision;d["controllerId"]=c.controllerId;d["boardProfile"]=c.boardProfile;customBoardJson(d["customBoard"].to<JsonObject>(),c.customBoard);
 auto i=d["identity"].to<JsonObject>();i["callSign"]=c.identity.callSign;i["postalCode"]=c.identity.postalCode;i["deviceName"]=c.identity.deviceName;i["description"]=c.identity.description;i["location"]=c.identity.location;i["hostName"]=c.identity.hostName;
 auto t=d["time"].to<JsonObject>();t["enabled"]=c.time.enabled;t["tz"]=c.time.tz;t["ntp1"]=c.time.ntp1;t["ntp2"]=c.time.ntp2;t["showLocal"]=c.time.showLocal;t["showUtc"]=c.time.showUtc;t["showDate"]=c.time.showDate;
 auto n=d["news"].to<JsonObject>();n["enabled"]=c.news.enabled;n["language"]=c.news.language;n["refreshMinutes"]=c.news.refreshMinutes;n["maxItems"]=c.news.maxItems;auto nf=n["feeds"].to<JsonArray>();for(auto&x:c.news.feeds){auto o=nf.add<JsonObject>();o["id"]=x.id;o["name"]=x.name;o["language"]=x.language;o["url"]=x.url;o["enabled"]=x.enabled;}
 auto f=d["federation"].to<JsonObject>();f["enabled"]=c.federation.enabled;f["systemName"]=c.federation.systemName;f["role"]=c.federation.role;f["admissionMode"]=c.federation.admissionMode;
 auto sec=d["security"].to<JsonObject>();sec["adminAuthEnabled"]=c.security.adminAuthEnabled;sec["adminUser"]=c.security.adminUser;sec["adminPassword"]=includeSecrets?c.security.adminPassword:"";
 auto tx=d["txInterlock"].to<JsonObject>();tx["enabled"]=c.txInterlock.enabled;tx["gpio"]=c.txInterlock.gpio;tx["activeHigh"]=c.txInterlock.activeHigh;
 auto fe=d["features"].to<JsonObject>();fe["externalApi"]=c.features.externalApi;fe["ota"]=c.features.ota;uiJson(d["ui"].to<JsonObject>(),c.ui);
 auto rs=d["relays"].to<JsonArray>();for(auto&r:c.relays){auto o=rs.add<JsonObject>();o["id"]=r.id;o["name"]=r.name;o["gpio"]=r.gpio;o["activeLow"]=r.activeLow;o["enabled"]=r.enabled;}
 auto fs=d["functions"].to<JsonArray>();for(auto&x:c.functions){auto o=fs.add<JsonObject>();o["id"]=x.id;o["label"]=x.label;o["type"]=x.type==FunctionType::Timed?"timed":(x.type==FunctionType::Storm?"storm":(x.type==FunctionType::Toggle?"toggle":"antenna"));o["relayId"]=x.relayId;o["enabled"]=x.enabled;o["visible"]=x.visible;o["durationMs"]=x.durationMs;o["requiresFunctionId"]=x.requiresFunctionId;o["group"]=x.group;o["stateToken"]=x.stateToken;}
 auto ds=d["devices"].to<JsonArray>();for(const auto&x:c.devices)deviceJson(ds.add<JsonObject>(),x);
}
static void sharedJson(JsonDocument&d,const SharedConfig&s,bool includeSecrets=false){d["schema"]=s.schema;d["revision"]=s.revision;d["revisionOrigin"]=s.revisionOrigin;d["systemTitle"]=s.systemTitle;d["systemStormMode"]=s.systemStormMode;auto sec=d["security"].to<JsonObject>();sec["adminAuthEnabled"]=s.security.adminAuthEnabled;sec["adminUser"]=s.security.adminUser;sec["adminPassword"]=includeSecrets?s.security.adminPassword:"";uiJson(d["ui"].to<JsonObject>(),s.ui);auto a=d["layout"].to<JsonArray>();for(auto&x:s.layout){auto o=a.add<JsonObject>();o["key"]=x.key;o["x"]=x.x;o["y"]=x.y;o["w"]=x.w;o["h"]=x.h;o["fontPx"]=x.fontPx;o["visible"]=x.visible;}auto dg=d["displayGroups"].to<JsonArray>();for(const auto&x:s.displayGroups)displayGroupJson(dg.add<JsonObject>(),x);auto rt=d["routes"].to<JsonArray>();for(const auto&x:s.routes)routeJson(rt.add<JsonObject>(),x);}
static void uiFrom(JsonVariantConst q,UiDefaults&v){v.buttonWidth=q["buttonWidth"]|170;v.buttonHeight=q["buttonHeight"]|72;v.defaultCols=constrain((int)(q["defaultCols"]|3),1,12);v.defaultRows=constrain((int)(q["defaultRows"]|1),1,6);v.fontPx=q["fontPx"]|20;v.normalColor=String((const char*)(q["normalColor"]|"#275c91"));v.activeColor=String((const char*)(q["activeColor"]|"#259b55"));v.powerInactiveColor=String((const char*)(q["powerInactiveColor"]|"#275c91"));v.powerActiveColor=String((const char*)(q["powerActiveColor"]|"#259b55"));v.lockedColor=String((const char*)(q["lockedColor"]|"#555b65"));v.rememberedColor=String((const char*)(q["rememberedColor"]|"#b83232"));v.runningColor=String((const char*)(q["runningColor"]|"#d67d00"));}

static bool sameRelays(const std::vector<RelayConfig>&a,const std::vector<RelayConfig>&b){
  if(a.size()!=b.size())return false;
  for(size_t i=0;i<a.size();++i){
    if(a[i].id!=b[i].id||a[i].gpio!=b[i].gpio||a[i].activeLow!=b[i].activeLow||a[i].enabled!=b[i].enabled)return false;
  }
  return true;
}
static bool sameFunctions(const std::vector<FunctionConfig>&a,const std::vector<FunctionConfig>&b){
  if(a.size()!=b.size())return false;
  for(size_t i=0;i<a.size();++i){
    if(a[i].id!=b[i].id||a[i].type!=b[i].type||a[i].relayId!=b[i].relayId||a[i].enabled!=b[i].enabled||
       a[i].requiresFunctionId!=b[i].requiresFunctionId||a[i].group!=b[i].group)return false;
  }
  return true;
}
static bool sameSubnet(IPAddress a,IPAddress b,IPAddress mask){for(int i=0;i<4;i++)if((a[i]&mask[i])!=(b[i]&mask[i]))return false;return true;}
static bool onboardingRequestOk(WebServer&server){return server.header("X-Ant-Controller")=="ANTCTRL3-ONBOARD"&&sameSubnet(server.client().remoteIP(),WiFi.localIP(),WiFi.subnetMask());}
static bool federationRequestOk(WebServer&server,LocalConfig*c,FederationService*fed){
 if(!c||!fed||!c->federation.enabled||c->federation.systemId.isEmpty()||server.header("X-Ant-Controller")!="ANTCTRL3"||!sameSubnet(server.client().remoteIP(),WiFi.localIP(),WiFi.subnetMask()))return false;
 if(server.header("X-Ant-System")!=c->federation.systemId)return false;String from=server.header("X-Ant-From");if(from.isEmpty())return false;
 return fed->authorizedCoordinator(from);
}
static bool federationPeerRequestOk(WebServer&server,LocalConfig*c,FederationService*fed){
 if(!c||!fed||!c->federation.enabled||c->federation.systemId.isEmpty()||server.header("X-Ant-Controller")!="ANTCTRL3"||!sameSubnet(server.client().remoteIP(),WiFi.localIP(),WiFi.subnetMask()))return false;
 if(server.header("X-Ant-System")!=c->federation.systemId)return false;String from=server.header("X-Ant-From");if(from.isEmpty())return false;if(from==c->controllerId)return true;
 for(const auto&p:fed->peers())if(p.controllerId==from&&p.online&&fed->sameSystem(p))return true;return false;
}

void WebUi::begin(LocalConfig*c,SharedConfig*s,Storage*st,RelayEngine*r,TimeService*t,FederationService*f,NewsService*n,WeatherService*weather,WifiManager*w,bool rec){
  const char* hdrs[]={"X-Ant-Controller","X-Ant-System","X-Ant-From","If-None-Match","Accept-Encoding"};server_.collectHeaders(hdrs,5);c_=c;s_=s;store_=st;rel_=r;time_=t;fed_=f;news_=n;weather_=weather;wifiMgr_=w;recovery_=rec;if(!recovery_)removeBundledUiFromLittleFs();recovery_?setupRoutes():routes();server_.begin();}
void WebUi::loop(){server_.handleClient();}
bool WebUi::auth(bool){const SecurityConfig&sec=s_?s_->security:c_->security;if(!sec.adminAuthEnabled)return true;if(server_.authenticate(sec.adminUser.c_str(),sec.adminPassword.c_str()))return true;server_.requestAuthentication();return false;}
void WebUi::sendJson(JsonDocument&d,int code){String x;serializeJson(d,x);server_.send(code,"application/json; charset=utf-8",x);}

void WebUi::setupRoutes(){
 auto portal=[this](){File f=LittleFS.open("/setup.html","r");if(!f){server_.send(500,"text/plain; charset=utf-8",c_->language=="en"?"Setup UI missing":"Setup-Oberfläche fehlt");return;}server_.streamFile(f,"text/html; charset=utf-8");f.close();};
 server_.on("/",HTTP_GET,portal);
 server_.on("/setup/scan",HTTP_GET,[this](){setupScan();});
 server_.on("/setup/saved",HTTP_GET,[this](){setupSaved();});
 server_.on("/setup/add",HTTP_POST,[this](){setupAdd();});
 server_.on("/setup/delete",HTTP_POST,[this](){setupDelete();});
 server_.on("/setup/move",HTTP_POST,[this](){setupMove();});
 server_.on("/generate_204",HTTP_ANY,portal);
 server_.on("/hotspot-detect.html",HTTP_ANY,portal);
 server_.on("/ncsi.txt",HTTP_ANY,portal);
 server_.on("/connecttest.txt",HTTP_ANY,portal);
 server_.on("/fwlink",HTTP_ANY,portal);
 server_.onNotFound([this](){server_.sendHeader("Location","http://192.168.4.1/",true);server_.send(302,"text/plain","");});
}
void WebUi::routes(){
 server_.on("/",HTTP_GET,[this](){if(loadUiBundle()){if(!sendBundledUi(server_,"index.html","text/html; charset=utf-8"))sendUiEncodingError(server_);return;}File f=LittleFS.open("/index.html","r");if(!f){server_.send(500,"text/plain; charset=utf-8",c_->language=="en"?"UI missing":"Oberfläche fehlt");return;}server_.sendHeader("Cache-Control","no-cache, must-revalidate");server_.streamFile(f,"text/html; charset=utf-8");f.close();});
 server_.on("/responsive.css",HTTP_GET,[this](){if(loadUiBundle()){if(!sendBundledUi(server_,"responsive.css","text/css; charset=utf-8"))sendUiEncodingError(server_);return;}File f=LittleFS.open("/responsive.css","r");if(!f){server_.send(404,"text/plain; charset=utf-8","responsive.css fehlt");return;}server_.sendHeader("Cache-Control","public, max-age=31536000, immutable");server_.streamFile(f,"text/css; charset=utf-8");f.close();});
 server_.on("/app.js",HTTP_GET,[this](){if(loadUiBundle()){if(!sendBundledUi(server_,"app.js","application/javascript; charset=utf-8"))sendUiEncodingError(server_);return;}File f=LittleFS.open("/app.js","r");if(!f){server_.send(500,"text/plain; charset=utf-8",c_->language=="en"?"app.js missing":"app.js fehlt");return;}server_.sendHeader("Cache-Control","public, max-age=31536000, immutable");server_.streamFile(f,"application/javascript; charset=utf-8");f.close();});
 server_.on("/style.css",HTTP_GET,[this](){if(loadUiBundle()){if(!sendBundledUi(server_,"style.css","text/css; charset=utf-8"))sendUiEncodingError(server_);return;}File f=LittleFS.open("/style.css","r");if(!f){server_.send(500,"text/plain; charset=utf-8",c_->language=="en"?"style.css missing":"style.css fehlt");return;}server_.sendHeader("Cache-Control","public, max-age=31536000, immutable");server_.streamFile(f,"text/css; charset=utf-8");f.close();});
 server_.serveStatic("/logo.png",LittleFS,"/logo.png","max-age=604800");
 server_.serveStatic("/favicon.ico",LittleFS,"/favicon.ico","max-age=604800");
 server_.on("/api/snapshot",HTTP_GET,[this](){apiSnapshot();});
 server_.on("/api/config",HTTP_GET,[this](){if(auth())apiConfigGet();});
 server_.on("/api/config",HTTP_PUT,[this](){if(auth())apiConfigPut();});
 server_.on("/api/shared",HTTP_GET,[this](){apiSharedGet();});
 server_.on("/api/shared",HTTP_PUT,[this](){if(auth())apiSharedPut();});
 server_.on("/api/layout/item",HTTP_PUT,[this](){if(auth())apiLayoutItemPut();});
 server_.on("/api/board",HTTP_GET,[this](){apiBoard();});
 server_.on("/api/execute",HTTP_POST,[this](){apiExecute();});
 server_.on("/api/route/activate",HTTP_POST,[this](){apiRouteActivate();});
 server_.on("/api/route/deactivate",HTTP_POST,[this](){apiRouteDeactivate();});
 server_.on("/api/system/storm",HTTP_POST,[this](){apiSystemStorm();});
 server_.on("/api/federation/storm",HTTP_POST,[this](){apiFederationStorm();});
 server_.on("/api/federation/group/off",HTTP_POST,[this](){apiFederationGroupOff();});
 server_.on("/api/stop",HTTP_POST,[this](){apiStop();});
 server_.on("/api/system/admission",HTTP_POST,[this](){if(auth())apiSystemAdmission();});
 server_.on("/api/provisioning/authorize",HTTP_POST,[this](){if(auth())apiProvisionAuthorize();});
 server_.on("/api/system/adopt",HTTP_POST,[this](){if(auth())apiSystemAdopt();});
 server_.on("/api/system/make-master",HTTP_POST,[this](){if(auth())apiSystemMakeMaster();});
 server_.on("/api/federation/adopt",HTTP_POST,[this](){apiFederationAdopt();});
 server_.on("/api/federation/make-master",HTTP_POST,[this](){apiFederationMakeMaster();});
 server_.on("/api/federation/stop",HTTP_POST,[this](){apiFederationStop();});
 server_.on("/api/test-relay",HTTP_POST,[this](){if(auth())apiTestRelay();});
 server_.on("/api/factory-reset",HTTP_POST,[this](){if(auth())apiFactoryReset();});
 server_.on("/api/controller/test-relay",HTTP_POST,[this](){if(auth())apiControllerTestRelay();});
 server_.on("/api/controller/test-gpio",HTTP_POST,[this](){if(auth())apiControllerTestGpio();});
 server_.on("/api/controller/restart",HTTP_POST,[this](){if(auth())apiControllerRestart();});
 server_.on("/api/controller/factory-reset",HTTP_POST,[this](){if(auth())apiControllerFactoryReset();});
 server_.on("/api/federation/test-relay",HTTP_POST,[this](){apiFederationTestRelay();});
 server_.on("/api/federation/test-gpio",HTTP_POST,[this](){apiFederationTestGpio();});
 server_.on("/api/federation/restart",HTTP_POST,[this](){apiFederationRestart();});
 server_.on("/api/federation/factory-reset",HTTP_POST,[this](){apiFederationFactoryReset();});
 server_.on("/api/restart",HTTP_POST,[this](){if(auth())apiRestart();});
 server_.on("/api/controller/config",HTTP_GET,[this](){if(auth())apiControllerConfigGet();});
 server_.on("/api/controller/config",HTTP_PUT,[this](){if(auth())apiControllerConfigPut();});
 server_.on("/api/federation/config",HTTP_GET,[this](){apiFederationConfigGet();});
 server_.on("/api/federation/config",HTTP_PUT,[this](){apiFederationConfigPut();});
 server_.on("/ext/status",HTTP_GET,[this](){if(!c_->features.externalApi){server_.send(404);return;}apiSnapshot();});
 server_.on("/ext/execute",HTTP_POST,[this](){if(!c_->features.externalApi){server_.send(404);return;}apiExecute();});
 server_.on("/api/peers",HTTP_GET,[this](){apiPeers();});
 server_.on("/api/combined",HTTP_GET,[this](){apiCombined();});
 server_.on("/api/time",HTTP_GET,[this](){server_.sendHeader("Cache-Control","no-store");JsonDocument d;d["localTime"]=time_->local();d["utcTime"]=time_->utc();d["showLocal"]=c_->time.showLocal;d["showUtc"]=c_->time.showUtc;d["showDate"]=c_->time.showDate;sendJson(d);});
 server_.on("/api/errors",HTTP_GET,[this](){if(auth())apiErrors();});
 server_.on("/api/errors",HTTP_DELETE,[this](){if(auth()){store_->clearErrors();server_.send(204);}});
 server_.on("/api/news",HTTP_GET,[this](){apiNews();});
 server_.on("/api/news/refresh",HTTP_POST,[this](){if(!auth())return;if(!fed_->isCoordinator()){server_.send(409,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Only the active master fetches news\"}":"{\"ok\":false,\"error\":\"Nur der aktive Master ruft Meldungen ab\"}");return;}news_->requestRefresh();server_.send(200,"application/json","{\"ok\":true}");});
 server_.on("/api/weather",HTTP_GET,[this](){apiWeather();});
 server_.on("/api/wifi/restore",HTTP_POST,[this](){if(!auth())return;JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json","{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}if(String((const char*)(d["format"]|""))!="AntennaControllerWifiBackupV1"){server_.send(409,"application/json","{\"ok\":false,\"error\":\"WLAN-Sicherungsformat ungültig\"}");return;}std::vector<WifiCredential> nets;for(JsonObject q:d["wifi"].as<JsonArray>()){WifiCredential w;w.ssid=String((const char*)(q["ssid"]|""));w.password=String((const char*)(q["password"]|""));nets.push_back(w);}String e;if(!store_->replaceWifiNetworks(nets,e)){JsonDocument r;r["ok"]=false;r["error"]=e;sendJson(r,409);return;}JsonDocument r;r["ok"]=true;r["count"]=(int)store_->wifiNetworks().size();sendJson(r);});
 server_.on("/api/wifi",HTTP_POST,[this](){if(auth())apiWifi();});
 server_.on("/api/setup-mode",HTTP_POST,[this](){if(auth())apiEnterSetupMode();});
 server_.on("/api/wifi/saved",HTTP_GET,[this](){if(auth())setupSaved();});
 server_.on("/api/federation/pull-shared",HTTP_POST,[this](){apiPullShared();});
 server_.on("/api/update/check",HTTP_GET,[this](){if(auth())sendOnlineUpdateCheck(server_,c_);});
 server_.on("/api/update-backup",HTTP_GET,[this](){if(auth())sendUpdateSafetyBackup(server_,store_,c_,String(ANTCTRL_VERSION));});
 server_.on("/api/update/install-latest",HTTP_POST,[this](){if(auth())otaInstallLatest();});
 server_.on("/api/ui-upload",HTTP_POST,[this](){if(!auth())return;JsonDocument r;r["ok"]=uiUploadOk;if(!uiUploadOk)r["error"]=uiUploadError;else r["file"]=uiUploadTarget;sendJson(r,uiUploadOk?200:409);},[this](){if(auth())handleUiUpload(server_.upload(),store_);});
 server_.on("/update",HTTP_GET,[this](){if(auth())otaPage();});
 server_.on("/rollback",HTTP_POST,[this](){if(auth())otaRollback();});
 server_.onNotFound([this](){server_.send(404,"text/plain",c_->language=="en"?"Not found":"Nicht gefunden");});
}

void WebUi::apiSnapshot(){
 JsonDocument d;d["controllerId"]=c_->controllerId;d["language"]=c_->language;d["firmwareVersion"]=ANTCTRL_VERSION;d["apiVersion"]=ANTCTRL_API_VERSION;d["buildDate"]=ANTCTRL_BUILD_DATE;d["name"]=c_->identity.deviceName;d["displayName"]=controllerDisplayName(c_->identity.deviceName,c_->federation.role,c_->federation.failoverPriority);d["callSign"]=c_->identity.callSign;d["postalCode"]=c_->identity.postalCode;d["description"]=c_->identity.description;d["location"]=c_->identity.location;d["ip"]=WiFi.localIP().toString();d["rssi"]=WiFi.RSSI();d["heap"]=ESP.getFreeHeap();d["sdk"]=ESP.getSdkVersion();d["uptimeMs"]=millis();d["resetReason"]=(int)esp_reset_reason();d["systemRole"]=c_->federation.role;d["systemName"]=c_->federation.systemName;d["coordinator"]=fed_->isCoordinator();d["temporaryCoordinator"]=fed_->isTemporaryCoordinator();d["reclaiming"]=fed_->reclaiming();d["permanentMasterOnline"]=fed_->permanentMasterOnline();d["masterOnline"]=fed_->masterOnline();d["masterIp"]=fed_->masterIp();d["localTime"]=time_->local();d["utcTime"]=time_->utc();d["timeSynced"]=time_->synced();auto&st=rel_->state();d["activeAntenna"]=st.activeAntennaId;auto abg=d["activeByGroup"].to<JsonObject>();for(const auto&a:st.activeSelections)abg[a.group]=a.functionId;d["polarization"]=st.polarization;d["motorRunning"]=st.motorRunning;d["motorFunctionId"]=st.motorFunctionId;d["motorRemainingMs"]=rel_->motorRemainingMs();d["stormMode"]=st.stormMode;d["systemStormMode"]=s_->systemStormMode;d["txActive"]=rel_->txActive();
 auto fs=d["functions"].to<JsonArray>();for(auto&x:c_->functions)if(x.enabled&&x.visible){auto o=fs.add<JsonObject>();o["id"]=x.id;o["label"]=x.label;o["type"]=x.type==FunctionType::Timed?"timed":(x.type==FunctionType::Storm?"storm":(x.type==FunctionType::Toggle?"toggle":"antenna"));o["requires"]=x.requiresFunctionId;o["group"]=x.group;o["stateToken"]=x.stateToken;o["durationMs"]=x.durationMs;bool configured=x.type==FunctionType::Storm;for(const auto&r:c_->relays)if(r.enabled&&r.id==x.relayId)configured=true;o["configured"]=configured;}
 auto ds=d["devices"].to<JsonArray>();for(const auto&x:c_->devices)if(x.enabled)deviceJson(ds.add<JsonObject>(),x);
 sendJson(d);
}
void WebUi::apiConfigGet(){JsonDocument d;cfgJson(d,*c_,true);sendJson(d);}
void WebUi::apiConfigPut(){
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",(c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}"));return;}
 bool force=server_.arg("force")=="1";uint32_t incomingRevision=d["revision"]|0U;
 if(!force && incomingRevision!=c_->revision){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Configuration was changed in the meantime. Please reload.":"Konfiguration wurde zwischenzeitlich geändert. Bitte neu laden.";r["currentRevision"]=c_->revision;sendJson(r,409);return;}
 if(!d["relays"].is<JsonArray>()||!d["functions"].is<JsonArray>()||!d["devices"].is<JsonArray>()){
  JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Configuration save refused: relay, function or device list is missing. Reload the complete configuration.":"Speichern abgebrochen: Relais-, Funktions- oder Geräteliste fehlt. Bitte die vollständige Konfiguration neu laden.";sendJson(r,409);return;
 }
 if((!c_->relays.empty()&&d["relays"].as<JsonArray>().size()==0)||(!c_->functions.empty()&&d["functions"].as<JsonArray>().size()==0)||(!c_->devices.empty()&&d["devices"].as<JsonArray>().size()==0)){
  JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Configuration save refused: an existing relay, function or device list would be erased.":"Speichern abgebrochen: Eine vorhandene Relais-, Funktions- oder Geräteliste würde vollständig gelöscht.";sendJson(r,409);return;
 }
 LocalConfig n=*c_;
 n.schema=11;
 n.language=String((const char*)(d["language"]|n.language.c_str()));
 n.boardProfile=String((const char*)(d["boardProfile"]|n.boardProfile.c_str()));customBoardFromJson(d["customBoard"],n.customBoard);JsonObject i=d["identity"].as<JsonObject>();n.identity.callSign=String((const char*)(i["callSign"]|n.identity.callSign.c_str()));n.identity.postalCode=String((const char*)(i["postalCode"]|n.identity.postalCode.c_str()));n.identity.deviceName=String((const char*)(i["deviceName"]|n.identity.deviceName.c_str()));n.identity.description=String((const char*)(i["description"]|n.identity.description.c_str()));n.identity.location=String((const char*)(i["location"]|n.identity.location.c_str()));n.identity.hostName=String((const char*)(i["hostName"]|n.identity.hostName.c_str()));
 JsonObject t=d["time"].as<JsonObject>();n.time.enabled=t["enabled"]|n.time.enabled;n.time.tz=String((const char*)(t["tz"]|n.time.tz.c_str()));n.time.ntp1=String((const char*)(t["ntp1"]|n.time.ntp1.c_str()));n.time.ntp2=String((const char*)(t["ntp2"]|n.time.ntp2.c_str()));n.time.showLocal=t["showLocal"]|n.time.showLocal;n.time.showUtc=t["showUtc"]|n.time.showUtc;n.time.showDate=t["showDate"]|n.time.showDate;
 JsonObject nw=d["news"].as<JsonObject>();n.news.enabled=nw["enabled"]|false;n.news.language=String((const char*)(nw["language"]|"de"));n.news.refreshMinutes=nw["refreshMinutes"]|30;n.news.maxItems=nw["maxItems"]|5;n.news.feeds.clear();for(JsonObject q:nw["feeds"].as<JsonArray>()){NewsFeed z;z.id=String((const char*)(q["id"]|""));z.name=String((const char*)(q["name"]|""));z.language=String((const char*)(q["language"]|"de"));z.url=String((const char*)(q["url"]|""));z.enabled=q["enabled"]|true;n.news.feeds.push_back(z);}
 JsonObject g=d["federation"].as<JsonObject>();n.federation.enabled=g["enabled"]|n.federation.enabled;n.federation.systemId=String((const char*)(g["systemId"]|n.federation.systemId.c_str()));n.federation.systemName=String((const char*)(g["systemName"]|n.federation.systemName.c_str()));n.federation.udpPort=g["udpPort"]|n.federation.udpPort;n.federation.role=String((const char*)(g["role"]|n.federation.role.c_str()));n.federation.permanentMasterId=String((const char*)(g["permanentMasterId"]|n.federation.permanentMasterId.c_str()));n.federation.admissionMode=String((const char*)(g["admissionMode"]|n.federation.admissionMode.c_str()));n.federation.failoverPriority=g["failoverPriority"]|n.federation.failoverPriority;
 JsonObject sec=d["security"].as<JsonObject>();n.security.adminAuthEnabled=sec["adminAuthEnabled"]|false;n.security.adminUser=String((const char*)(sec["adminUser"]|"admin"));n.security.adminPassword=String((const char*)(sec["adminPassword"]|""));
 JsonObject tx=d["txInterlock"].as<JsonObject>();n.txInterlock.enabled=tx["enabled"]|false;n.txInterlock.gpio=tx["gpio"]|34;n.txInterlock.activeHigh=tx["activeHigh"]|true;
 JsonObject fe=d["features"].as<JsonObject>();n.features.externalApi=fe["externalApi"]|true;n.features.ota=fe["ota"]|true;uiFrom(d["ui"],n.ui);
 n.relays.clear();for(JsonObject q:d["relays"].as<JsonArray>()){RelayConfig z;z.id=String((const char*)(q["id"]|""));z.name=String((const char*)(q["name"]|""));z.gpio=q["gpio"]|-1;z.activeLow=q["activeLow"]|true;z.enabled=q["enabled"]|false;n.relays.push_back(z);}
 n.functions.clear();for(JsonObject q:d["functions"].as<JsonArray>()){FunctionConfig z;z.id=String((const char*)(q["id"]|""));z.label=String((const char*)(q["label"]|""));String ftype=String((const char*)(q["type"]|"antenna"));z.type=ftype=="timed"?FunctionType::Timed:(ftype=="storm"?FunctionType::Storm:(ftype=="toggle"?FunctionType::Toggle:FunctionType::Antenna));z.relayId=String((const char*)(q["relayId"]|""));z.enabled=q["enabled"]|true;z.visible=q["visible"]|true;z.durationMs=q["durationMs"]|7500;z.requiresFunctionId=String((const char*)(q["requiresFunctionId"]|""));z.group=String((const char*)(q["group"]|"ANT"));z.stateToken=String((const char*)(q["stateToken"]|""));n.functions.push_back(z);}
 n.devices.clear();for(JsonObjectConst q:d["devices"].as<JsonArrayConst>()){LogicalDeviceConfig z;deviceFromJson(q,z);n.devices.push_back(z);}
 bool hardwareChanged = n.boardProfile!=c_->boardProfile || !sameCustomBoard(n.customBoard,c_->customBoard) ||
    n.txInterlock.enabled!=c_->txInterlock.enabled || n.txInterlock.gpio!=c_->txInterlock.gpio ||
    n.txInterlock.activeHigh!=c_->txInterlock.activeHigh || !sameRelays(n.relays,c_->relays) || !sameFunctions(n.functions,c_->functions);
 bool hostChanged=n.identity.hostName!=c_->identity.hostName;
 bool timeChanged=n.time.enabled!=c_->time.enabled||n.time.tz!=c_->time.tz||n.time.ntp1!=c_->time.ntp1||n.time.ntp2!=c_->time.ntp2;
 bool fedChanged=n.federation.enabled!=c_->federation.enabled||n.federation.systemId!=c_->federation.systemId||n.federation.systemName!=c_->federation.systemName||n.federation.udpPort!=c_->federation.udpPort||n.federation.role!=c_->federation.role||n.federation.permanentMasterId!=c_->federation.permanentMasterId||n.federation.admissionMode!=c_->federation.admissionMode||n.federation.failoverPriority!=c_->federation.failoverPriority;
 bool weatherChanged=n.identity.postalCode!=c_->identity.postalCode||n.language!=c_->language;
 bool newsChanged=n.news.enabled!=c_->news.enabled||n.news.language!=c_->news.language||n.news.refreshMinutes!=c_->news.refreshMinutes||n.news.maxItems!=c_->news.maxItems||n.news.feeds.size()!=c_->news.feeds.size();
 if(!newsChanged){for(size_t i=0;i<n.news.feeds.size();++i){const auto&a=n.news.feeds[i];const auto&b=c_->news.feeds[i];if(a.id!=b.id||a.name!=b.name||a.language!=b.language||a.url!=b.url||a.enabled!=b.enabled){newsChanged=true;break;}}}
 String e;if(!store_->saveLocal(n,e)){Serial.println("Config save: "+e);JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Configuration could not be saved":e;sendJson(r,409);return;}
 if(hardwareChanged) rel_->emergencyOff();
 *c_=n;
 if(hardwareChanged) rel_->safeInit();
 bool hostnameApplied=true; // main.cpp reapplies the correct device/system mDNS name on the next loop
 if(timeChanged)time_->begin(c_->time);if(fedChanged)fed_->configChanged();if(newsChanged)news_->configChanged();if(weatherChanged&&weather_)weather_->configChanged();
 JsonDocument r;r["ok"]=true;r["revision"]=c_->revision;r["hardwareReset"]=hardwareChanged;r["hostnameApplied"]=hostnameApplied;sendJson(r);
}
void WebUi::apiSharedGet(){bool internal=server_.arg("internal")=="1"&&federationPeerRequestOk(server_,c_,fed_);JsonDocument d;sharedJson(d,*s_,internal);sendJson(d);}
static bool layoutSetValid(const SharedConfig&cfg,String&err);
void WebUi::apiSharedPut(){
 if(!fed_->isCoordinator()){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Shared system configuration is owned by the active master":"Die gemeinsame Anlagenkonfiguration wird vom aktiven Master verwaltet";sendJson(r,409);return;}
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}
 bool force=server_.arg("force")=="1";uint32_t incomingRevision=d["revision"]|0U;
 if(!force && incomingRevision!=s_->revision){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Shared layout was changed in the meantime. Please reload.":"Gemeinsame Anordnung wurde zwischenzeitlich geändert. Bitte neu laden.";r["currentRevision"]=s_->revision;sendJson(r,409);return;}
 if(!d["layout"].is<JsonArray>()||!d["displayGroups"].is<JsonArray>()||!d["routes"].is<JsonArray>()){
  JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Shared configuration save refused: layout, groups or signal paths are missing. Reload the complete configuration.":"Speichern abgebrochen: Anordnung, Gruppen oder Signalwege fehlen. Bitte die vollständige Anlagenkonfiguration neu laden.";sendJson(r,409);return;
 }
 bool routeShrink=d["routes"].as<JsonArray>().size()<s_->routes.size();
 bool explicitRouteShrink=server_.arg("allow-route-shrink")=="1";
 if((!s_->layout.empty()&&d["layout"].as<JsonArray>().size()==0)||(!s_->displayGroups.empty()&&d["displayGroups"].as<JsonArray>().size()==0)||(routeShrink&&!explicitRouteShrink)){
  JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Shared configuration save refused because it would erase existing layout, groups or signal paths. Confirm the specific deletion in the interface.":"Speichern abgebrochen: Vorhandene Anordnung, Gruppen oder Signalwege würden gelöscht. Bitte die konkrete Löschung in der Oberfläche ausdrücklich bestätigen.";sendJson(r,409);return;
 }
 SharedConfig n=*s_;n.schema=5;n.systemStormMode=s_->systemStormMode;n.systemTitle=String((const char*)(d["systemTitle"]|n.systemTitle.c_str()));JsonObject sec=d["security"].as<JsonObject>();n.security.adminAuthEnabled=sec["adminAuthEnabled"]|n.security.adminAuthEnabled;n.security.adminUser=String((const char*)(sec["adminUser"]|n.security.adminUser.c_str()));if(sec["adminPassword"].is<const char*>()){String pw=String((const char*)(sec["adminPassword"]|""));if(!pw.isEmpty()||!n.security.adminAuthEnabled)n.security.adminPassword=pw;}if(n.security.adminAuthEnabled&&n.security.adminPassword.isEmpty()){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Administrator password must not be empty":"Administrator-Passwort darf nicht leer sein";sendJson(r,409);return;}uiFrom(d["ui"],n.ui);n.layout.clear();for(JsonObject q:d["layout"].as<JsonArray>()){LayoutItem z;z.key=String((const char*)(q["key"]|""));z.w=constrain((int)(q["w"]|3),1,12);z.h=constrain((int)(q["h"]|1),1,6);z.x=constrain((int)(q["x"]|0),0,12-z.w);z.y=constrain((int)(q["y"]|0),0,6-z.h);z.fontPx=constrain((int)(q["fontPx"]|20),10,48);z.visible=q["visible"]|true;n.layout.push_back(z);}n.displayGroups.clear();for(JsonObjectConst q:d["displayGroups"].as<JsonArrayConst>()){DisplayGroupConfig z;displayGroupFromJson(q,z);if(!z.id.isEmpty()&&!z.title.isEmpty())n.displayGroups.push_back(z);}if(n.displayGroups.empty())n.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};n.routes.clear();for(JsonObjectConst q:d["routes"].as<JsonArrayConst>()){RouteConfig z;routeFromJson(q,z);if(!z.id.isEmpty())n.routes.push_back(z);}String e;if(!layoutSetValid(n,e)){JsonDocument r;r["ok"]=false;r["error"]=e;sendJson(r,409);return;}n.revision=s_->revision+1;n.revisionOrigin=c_->controllerId;if(!store_->saveShared(n,e,explicitRouteShrink)){Serial.println("Shared save: "+e);JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Shared layout could not be saved":"Gemeinsame Anordnung konnte nicht gespeichert werden";sendJson(r,409);return;}*s_=n;fed_->notifySharedChanged();JsonDocument r;sharedJson(r,*s_);r["ok"]=true;sendJson(r);
}
static bool layoutRectOverlap(const LayoutItem&a,const LayoutItem&b){
 return a.x<b.x+b.w && a.x+a.w>b.x && a.y<b.y+b.h && a.y+a.h>b.y;
}
static bool layoutSetValid(const SharedConfig&cfg,String&err){
 for(size_t i=0;i<cfg.layout.size();++i){
  const auto&a=cfg.layout[i];
  if(a.key.isEmpty()){err="Leerer Layout-Schlüssel";return false;}
  if(a.w<1||a.w>12||a.h<1||a.h>6||a.x<0||a.y<0||a.x+a.w>12||a.y+a.h>6){err="Layout außerhalb des 12×6-Rasters";return false;}
  if(!a.visible)continue;
  for(size_t j=i+1;j<cfg.layout.size();++j){const auto&b=cfg.layout[j];if(b.visible&&layoutRectOverlap(a,b)){err="Layoutflächen überlappen sich";return false;}}
 }
 return true;
}
static bool layoutPositionFree(const SharedConfig&cfg,size_t movingIndex,int x,int y,int w,int h){
 LayoutItem probe=cfg.layout[movingIndex];probe.x=x;probe.y=y;probe.w=w;probe.h=h;
 if(x<0||y<0||x+w>12||y+h>6)return false;
 for(size_t j=0;j<cfg.layout.size();++j){if(j==movingIndex)continue;const auto&o=cfg.layout[j];if(o.visible&&layoutRectOverlap(probe,o))return false;}
 return true;
}
static bool layoutOverlapsPlaced(const SharedConfig&cfg,size_t idx,const std::vector<size_t>&placed){
 const auto&item=cfg.layout[idx];
 for(size_t j:placed){if(cfg.layout[j].visible&&layoutRectOverlap(item,cfg.layout[j]))return true;}
 return false;
}
static bool layoutNormalizeAll(SharedConfig&cfg,size_t fixedA,size_t fixedB,bool&reflowed,String&err){
 reflowed=false;
 for(auto&item:cfg.layout){
  item.w=constrain(item.w,1,12);item.h=constrain(item.h,1,6);
  item.x=constrain(item.x,0,12-item.w);item.y=constrain(item.y,0,6-item.h);
  item.fontPx=constrain(item.fontPx,10,48);
 }
 std::vector<size_t> order;
 if(fixedA<cfg.layout.size())order.push_back(fixedA);
 if(fixedB<cfg.layout.size()&&fixedB!=fixedA)order.push_back(fixedB);
 for(size_t i=0;i<cfg.layout.size();++i)if(i!=fixedA&&i!=fixedB)order.push_back(i);
 std::vector<size_t> placed;
 for(size_t idx:order){
  auto&item=cfg.layout[idx];
  if(!item.visible)continue;
  if(!layoutOverlapsPlaced(cfg,idx,placed)){placed.push_back(idx);continue;}
  if(idx==fixedA||idx==fixedB){
   err="Die gewünschte Layoutposition überlappt eine fest gesetzte Fläche";
   return false;
  }
  bool found=false;
  int ox=item.x,oy=item.y;
  for(int y=0;y<=6-item.h&&!found;++y){
   for(int x=0;x<=12-item.w;++x){
    item.x=x;item.y=y;
    if(!layoutOverlapsPlaced(cfg,idx,placed)){found=true;break;}
   }
  }
  if(!found){
   item.x=ox;item.y=oy;
   err="Für eine überlappende Layoutfläche ist kein freier Rasterplatz vorhanden";
   return false;
  }
  if(item.x!=ox||item.y!=oy)reflowed=true;
  placed.push_back(idx);
 }
 return true;
}
void WebUi::apiLayoutItemPut(){
 if(!fed_->isCoordinator()){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Layout can only be changed on the active master":"Das Anlagenlayout kann nur am aktiven Master geändert werden";sendJson(r,409);return;}
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json; charset=utf-8",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}
 String key=String((const char*)(d["key"]|"")),swapKey=String((const char*)(d["swapKey"]|""));if(key.isEmpty()){server_.send(400,"application/json; charset=utf-8",c_->language=="en"?"{\"ok\":false,\"error\":\"Layout key missing\"}":"{\"ok\":false,\"error\":\"Layout-Schlüssel fehlt\"}");return;}
 SharedConfig n=*s_;size_t itemIndex=(size_t)-1,swapIndex=(size_t)-1;for(size_t i=0;i<n.layout.size();++i){if(n.layout[i].key==key)itemIndex=i;if(!swapKey.isEmpty()&&n.layout[i].key==swapKey)swapIndex=i;}
 if(itemIndex==(size_t)-1){server_.send(404,"application/json; charset=utf-8",c_->language=="en"?"{\"ok\":false,\"error\":\"Layout item not found\"}":"{\"ok\":false,\"error\":\"Layoutfläche nicht gefunden\"}");return;}
 LayoutItem&item=n.layout[itemIndex];int oldX=item.x,oldY=item.y;
 item.w=constrain((int)(d["w"]|item.w),1,12);item.h=constrain((int)(d["h"]|item.h),1,6);item.fontPx=constrain((int)(d["fontPx"]|item.fontPx),10,48);item.visible=d["visible"]|item.visible;
 int nx=constrain((int)(d["x"]|item.x),0,12-item.w),ny=constrain((int)(d["y"]|item.y),0,6-item.h);bool reflowed=false;String e;
 if(!swapKey.isEmpty()){
  if(swapIndex==(size_t)-1||swapIndex==itemIndex){server_.send(409,"application/json; charset=utf-8",c_->language=="en"?"{\"ok\":false,\"error\":\"Swap target not found\"}":"{\"ok\":false,\"error\":\"Tauschziel nicht gefunden\"}");return;}
  LayoutItem&swap=n.layout[swapIndex];int sx=swap.x,sy=swap.y;if(sx+item.w>12||sy+item.h>6||oldX+swap.w>12||oldY+swap.h>6){server_.send(409,"application/json; charset=utf-8",c_->language=="en"?"{\"ok\":false,\"error\":\"Items do not fit when swapped\"}":"{\"ok\":false,\"error\":\"Flächen passen beim Tauschen nicht in das Raster\"}");return;}
  item.x=sx;item.y=sy;swap.x=oldX;swap.y=oldY;
  if(!layoutNormalizeAll(n,itemIndex,swapIndex,reflowed,e)){server_.send(409,"application/json; charset=utf-8",String("{\"ok\":false,\"error\":\"")+e+"\"}");return;}
 }else{
  item.x=nx;item.y=ny;
  // Die gerade bearbeitete Fläche bleibt fest an ihrer neuen Position.
  // Bereits vorhandene Alt-Überlappungen oder Nachbarn im Weg werden vollständig
  // normalisiert und auf freie Rasterplätze verschoben, statt den ganzen Schreibvorgang
  // wegen einer unrelated alten Kollision mit HTTP 409 abzulehnen.
  if(!layoutNormalizeAll(n,itemIndex,(size_t)-1,reflowed,e)){server_.send(409,"application/json; charset=utf-8",String("{\"ok\":false,\"error\":\"")+e+"\"}");return;}
 }
 n.revision=s_->revision+1;n.revisionOrigin=c_->controllerId;if(!store_->saveShared(n,e)){Serial.println("Layout item save: "+e);JsonDocument r;r["ok"]=false;r["error"]=e;sendJson(r,409);return;}*s_=n;fed_->notifySharedChanged();
 JsonDocument r;r["ok"]=true;r["revision"]=s_->revision;r["revisionOrigin"]=s_->revisionOrigin;r["reflowed"]=reflowed;auto o=r["item"].to<JsonObject>();const auto&saved=s_->layout[itemIndex];o["key"]=saved.key;o["x"]=saved.x;o["y"]=saved.y;o["w"]=saved.w;o["h"]=saved.h;o["fontPx"]=saved.fontPx;o["visible"]=saved.visible;if(swapIndex!=(size_t)-1){const auto&sw=s_->layout[swapIndex];auto q=r["swapItem"].to<JsonObject>();q["key"]=sw.key;q["x"]=sw.x;q["y"]=sw.y;q["w"]=sw.w;q["h"]=sw.h;q["fontPx"]=sw.fontPx;q["visible"]=sw.visible;}sendJson(r);
}

void WebUi::apiBoard(){
 JsonDocument d;bool en=c_->language=="en";auto defs=d["profiles"].to<JsonArray>();
 for(auto&b:BoardProfiles::all()){
   auto bo=defs.add<JsonObject>();bo["id"]=b.id;bo["name"]=en?b.nameEn:b.nameDe;bo["note"]=en?b.noteEn:b.noteDe;bo["nameDe"]=b.nameDe;bo["nameEn"]=b.nameEn;bo["noteDe"]=b.noteDe;bo["noteEn"]=b.noteEn;
   auto ps=bo["pins"].to<JsonArray>();
   for(auto&p:b.pins){
     auto o=ps.add<JsonObject>();o["label"]=p.label;if(p.gpio>=0)o["gpio"]=p.gpio;o["side"]=p.side;o["order"]=p.order;o["note"]=en?p.noteEn:p.noteDe;o["noteDe"]=p.noteDe;o["noteEn"]=p.noteEn;
     const char*cl="forbidden";if(p.pinClass==PinClass::Recommended)cl="recommended";else if(p.pinClass==PinClass::Caution)cl="caution";else if(p.pinClass==PinClass::InputOnly)cl="input";else if(p.pinClass==PinClass::Power)cl="power";else if(p.pinClass==PinClass::Ground)cl="ground";else if(p.pinClass==PinClass::Control)cl="control";o["class"]=cl;
   }
 }
 sendJson(d);
}
void WebUi::apiRouteActivate(){
 if(!fed_->isCoordinator()){
  JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Only the active master may activate a signal path":"Nur der aktive Master darf einen Signalweg aktivieren";sendJson(r,409);return;
 }
 JsonDocument in;if(deserializeJson(in,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}
 String routeId=String((const char*)(in["id"]|""));RouteConfig* route=nullptr;for(auto&x:s_->routes)if(x.id==routeId){route=&x;break;}
 if(!route||!route->enabled){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Signal path is missing or disabled":"Signalweg fehlt oder ist deaktiviert";sendJson(r,404);return;}
 if(s_->systemStormMode){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"System storm mode is active":"Anlagen-Gewittermodus ist aktiv";sendJson(r,409);return;}
 if(route->deviceKeys.size()<2){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Signal path needs at least two system devices":"Signalweg benötigt mindestens zwei Anlagenteile";sendJson(r,409);return;}

 struct Req{String controllerId;String functionId;String group;};
 std::vector<Req> reqs;std::vector<String> cacheIds,cacheBodies,nodeIds,routeNodeIds,exclusiveKeys,exclusiveNames,routeSeenKeys,routeCategories;std::vector<std::vector<String>> knownBands;String error;auto peers=fed_->peers();
 auto peerById=[&](const String&cid)->PeerInfo*{for(auto&p:peers)if(p.controllerId==cid&&fed_->sameSystem(p))return &p;return nullptr;};
 auto remoteBody=[&](const String&cid,String&body)->bool{
  for(size_t i=0;i<cacheIds.size();++i)if(cacheIds[i]==cid){body=cacheBodies[i];return true;}
  PeerInfo*p=peerById(cid);if(!p||!p->online){error=c_->language=="en"?"Controller required by the signal path is offline":"Ein vom Signalweg benötigtes Steuergerät ist nicht erreichbar";return false;}
  if(!fed_->fetchRemoteSnapshot(*p,body)){error=c_->language=="en"?"Controller state could not be read":"Zustand eines Steuergeräts konnte nicht gelesen werden";return false;}
  cacheIds.push_back(cid);cacheBodies.push_back(body);return true;
 };
 auto addNode=[&](const String&cid){for(const auto&x:nodeIds)if(x==cid)return;nodeIds.push_back(cid);};
 auto addRouteNode=[&](const String&cid){for(const auto&x:routeNodeIds)if(x==cid)return;routeNodeIds.push_back(cid);};
 auto addReq=[&](const String&cid,const String&fid,const String&group)->bool{
  for(const auto&r:reqs){if(r.controllerId==cid&&r.group==group&&r.functionId!=fid){error=c_->language=="en"?"Signal path requires two different functions in the same switching group":"Signalweg verlangt im selben Steuergerät zwei verschiedene Funktionen derselben Schaltgruppe";return false;}if(r.controllerId==cid&&r.functionId==fid)return true;}
  Req r;r.controllerId=cid;r.functionId=fid;r.group=group;reqs.push_back(r);addNode(cid);return true;
 };
 auto addBands=[&](JsonVariantConst q){std::vector<String>b;for(JsonVariantConst v:q.as<JsonArrayConst>()){String x=String((const char*)(v|""));if(!x.isEmpty())b.push_back(x);}if(!b.empty())knownBands.push_back(b);};
 auto splitKey=[&](const String&key,String&cid,String&did)->bool{int p=key.indexOf(':');if(p<=0||p>=(int)key.length()-1)return false;cid=key.substring(0,p);did=key.substring(p+1);return !cid.isEmpty()&&!did.isEmpty();};

 for(const auto&key:route->deviceKeys){
  for(const auto&seen:routeSeenKeys)if(seen==key){error=c_->language=="en"?"The same system device occurs twice in the signal path":"Dasselbe Anlagenteil kommt im Signalweg doppelt vor";break;}if(!error.isEmpty())break;routeSeenKeys.push_back(key);
  String cid,did;if(!splitKey(key,cid,did)){error=c_->language=="en"?"Invalid device reference in signal path":"Ungültige Anlagenteil-Referenz im Signalweg";break;}
  addRouteNode(cid);
  if(cid==c_->controllerId){
   const LogicalDeviceConfig*dev=nullptr;for(const auto&d:c_->devices)if(d.enabled&&d.id==did){dev=&d;break;}if(!dev){error=c_->language=="en"?"System device referenced by signal path is missing":"Ein im Signalweg verwendetes Anlagenteil fehlt";break;}routeCategories.push_back(dev->category);
   if(!dev->bands.empty())knownBands.push_back(dev->bands);if(dev->exclusive){exclusiveKeys.push_back(key);exclusiveNames.push_back(dev->name);}
   for(const auto&fid:dev->functionIds){const FunctionConfig*f=nullptr;for(const auto&x:c_->functions)if(x.enabled&&x.visible&&x.id==fid){f=&x;break;}if(!f){error=c_->language=="en"?"System device references a missing switching function":"Anlagenteil verweist auf eine fehlende Schaltfunktion";break;}if(f->type==FunctionType::Antenna&&!addReq(cid,f->id,f->group.isEmpty()?String("ANT"):f->group))break;}
   if(!error.isEmpty())break;
  }else{
   String body;if(!remoteBody(cid,body))break;JsonDocument rd;if(deserializeJson(rd,body)){error=c_->language=="en"?"Invalid controller state received":"Ungültiger Steuergerätezustand empfangen";break;}
   bool devFound=false;for(JsonObjectConst dev:rd["devices"].as<JsonArrayConst>()){if(String((const char*)(dev["id"]|""))!=did)continue;devFound=true;routeCategories.push_back(String((const char*)(dev["category"]|"other")));addBands(dev["bands"]);if(dev["exclusive"]|false){exclusiveKeys.push_back(key);exclusiveNames.push_back(String((const char*)(dev["name"]|did.c_str())));}for(JsonVariantConst fv:dev["functionIds"].as<JsonArrayConst>()){String fid=String((const char*)(fv|""));bool fnFound=false;String type,group;for(JsonObjectConst fn:rd["functions"].as<JsonArrayConst>())if(String((const char*)(fn["id"]|""))==fid){fnFound=true;type=String((const char*)(fn["type"]|""));group=String((const char*)(fn["group"]|"ANT"));break;}if(!fnFound){error=c_->language=="en"?"System device references a missing switching function":"Anlagenteil verweist auf eine fehlende Schaltfunktion";break;}if(type=="antenna"&&!addReq(cid,fid,group))break;}break;}
   if(!devFound&&error.isEmpty())error=c_->language=="en"?"System device referenced by signal path is missing":"Ein im Signalweg verwendetes Anlagenteil fehlt";if(!error.isEmpty())break;
  }
 }
 if(error.isEmpty())for(size_t i=1;i<routeCategories.size();++i){const String&a=routeCategories[i-1];const String&b=routeCategories[i];if((a=="radio"&&b=="radio")||(a=="antenna"&&b=="antenna")){error=c_->language=="en"?"Adjacent radios or adjacent antennas are not a valid RF path":"Funkgerät direkt an Funkgerät bzw. Antenne direkt an Antenne ist kein gültiger HF-Weg";break;}}
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}
 if(reqs.empty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Signal path has no persistent switching function that can be confirmed":"Signalweg enthält keine dauerhaft schaltbare Funktion, deren Zustand bestätigt werden kann";sendJson(r,409);return;}

 auto containsKey=[](const std::vector<String>&v,const String&key){for(const auto&x:v)if(x==key)return true;return false;};
 auto quietRemoteBody=[&](const String&cid,String&body)->bool{for(size_t i=0;i<cacheIds.size();++i)if(cacheIds[i]==cid){body=cacheBodies[i];return true;}PeerInfo*p=peerById(cid);if(!p||!p->online)return false;if(!fed_->fetchRemoteSnapshot(*p,body))return false;cacheIds.push_back(cid);cacheBodies.push_back(body);return true;};
 auto routeActiveNow=[&](const RouteConfig&rr)->bool{bool hasPersistent=false;for(const auto&key:rr.deviceKeys){String cid,did;if(!splitKey(key,cid,did))return false;if(cid==c_->controllerId){const LogicalDeviceConfig*dev=nullptr;for(const auto&d:c_->devices)if(d.enabled&&d.id==did){dev=&d;break;}if(!dev)return false;for(const auto&fid:dev->functionIds){const FunctionConfig*f=nullptr;for(const auto&x:c_->functions)if(x.enabled&&x.visible&&x.id==fid){f=&x;break;}if(f&&f->type==FunctionType::Antenna){hasPersistent=true;if(!rel_->isFunctionActive(f->id))return false;}}}else{String body;if(!quietRemoteBody(cid,body))return false;JsonDocument rd;if(deserializeJson(rd,body))return false;JsonObjectConst found;for(JsonObjectConst dev:rd["devices"].as<JsonArrayConst>())if(String((const char*)(dev["id"]|""))==did){found=dev;break;}if(found.isNull())return false;for(JsonVariantConst fv:found["functionIds"].as<JsonArrayConst>()){String fid=String((const char*)(fv|""));for(JsonObjectConst fn:rd["functions"].as<JsonArrayConst>())if(String((const char*)(fn["id"]|""))==fid&&String((const char*)(fn["type"]|""))=="antenna"){hasPersistent=true;String group=String((const char*)(fn["group"]|"ANT"));const char*active=rd["activeByGroup"][group.c_str()];if(!active||String(active)!=fid)return false;break;}}}}return hasPersistent;};
 for(size_t i=0;i<exclusiveKeys.size()&&error.isEmpty();++i){for(const auto&other:s_->routes){if(other.id==route->id||other.enabled==false||!containsKey(other.deviceKeys,exclusiveKeys[i]))continue;if(routeActiveNow(other)){error=(c_->language=="en"?"Exclusive system device is already used by active signal path: ":"Exklusives Anlagenteil wird bereits von einem aktiven Signalweg verwendet: ")+exclusiveNames[i];break;}}}
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}

 // Vor dem Einschalten eines Signalwegs müssen alle anderen Wege vollständig AUS sein.
 // Die Oberfläche schaltet den bisherigen Weg zuerst ab; diese Prüfung schützt auch
 // direkte API-Aufrufe und verhindert parallele Verbindungen aus verschiedenen Gruppen.
 auto collectRouteReqs=[&](const RouteConfig&rr,std::vector<Req>&out)->bool{
  out.clear();for(const auto&key:rr.deviceKeys){String cid,did;if(!splitKey(key,cid,did))return false;
   if(cid==c_->controllerId){const LogicalDeviceConfig*dev=nullptr;for(const auto&d:c_->devices)if(d.enabled&&d.id==did){dev=&d;break;}if(!dev)return false;for(const auto&fid:dev->functionIds){const FunctionConfig*f=nullptr;for(const auto&x:c_->functions)if(x.enabled&&x.visible&&x.id==fid){f=&x;break;}if(f&&f->type==FunctionType::Antenna){Req q{cid,f->id,f->group.isEmpty()?String("ANT"):f->group};bool have=false;for(const auto&z:out)if(z.controllerId==q.controllerId&&z.group==q.group&&z.functionId==q.functionId){have=true;break;}if(!have)out.push_back(q);}}}
   else{String body;if(!quietRemoteBody(cid,body))return false;JsonDocument rd;if(deserializeJson(rd,body))return false;JsonObjectConst dev;for(JsonObjectConst d:rd["devices"].as<JsonArrayConst>())if(String((const char*)(d["id"]|""))==did){dev=d;break;}if(dev.isNull())return false;for(JsonVariantConst fv:dev["functionIds"].as<JsonArrayConst>()){String fid=String((const char*)(fv|""));for(JsonObjectConst fn:rd["functions"].as<JsonArrayConst>())if(String((const char*)(fn["id"]|""))==fid&&String((const char*)(fn["type"]|""))=="antenna"){Req q{cid,fid,String((const char*)(fn["group"]|"ANT"))};bool have=false;for(const auto&z:out)if(z.controllerId==q.controllerId&&z.group==q.group&&z.functionId==q.functionId){have=true;break;}if(!have)out.push_back(q);break;}}}
  }return true;};
 for(const auto&other:s_->routes){
  if(!error.isEmpty())break;
  if(other.id==route->id||other.enabled==false)continue;
  std::vector<Req>otherReqs;
  if(!collectRouteReqs(other,otherReqs)){
   error=c_->language=="en"?"Cannot verify that every other signal path is off; check controller connectivity":"Es kann nicht sicher geprüft werden, ob alle anderen Signalwege AUS sind. Bitte Steuergeräteverbindung prüfen";
   break;
  }
  for(const auto&otherReq:otherReqs){
   bool active=false;
   if(otherReq.controllerId==c_->controllerId)active=rel_->isFunctionActive(otherReq.functionId);
   else{
    String body;
    if(!quietRemoteBody(otherReq.controllerId,body)){
     error=c_->language=="en"?"Cannot verify that every other signal path is off; check controller connectivity":"Es kann nicht sicher geprüft werden, ob alle anderen Signalwege AUS sind. Bitte Steuergeräteverbindung prüfen";
     break;
    }
    JsonDocument rd;
    if(deserializeJson(rd,body)){
     error=c_->language=="en"?"Cannot verify the state of another signal path":"Der Zustand eines anderen Signalwegs kann nicht sicher geprüft werden";
     break;
    }
    const char*activeId=rd["activeByGroup"][otherReq.group.c_str()];
    active=activeId&&String(activeId)==otherReq.functionId;
   }
   if(active){
    error=c_->language=="en"?"Another signal path is active. Only one radio-to-antenna connection may be active at a time":"Ein anderer Signalweg ist aktiv. Es darf immer nur eine Verbindung von einem Funkgerät zu einer Antenne aktiv sein";
    break;
   }
  }
 }
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}
 for(const auto&other:s_->routes){if(!error.isEmpty())break;if(other.id==route->id||other.enabled==false||!routeActiveNow(other))continue;std::vector<Req>ors;if(!collectRouteReqs(other,ors))continue;for(const auto&nr:reqs){for(const auto&ar:ors){if(nr.controllerId==ar.controllerId&&nr.group==ar.group&&nr.functionId!=ar.functionId){error=(c_->language=="en"?"Switching group is already occupied by active signal path ":"Schaltgruppe ist bereits durch aktiven Signalweg belegt: ")+(other.label.isEmpty()?other.id:other.label)+" · "+nr.group;break;}}if(!error.isEmpty())break;}}
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}

 String bandStatus="unknown";std::vector<String>common;if(knownBands.size()>=2){common=knownBands[0];for(size_t i=1;i<knownBands.size();++i){std::vector<String>next;for(const auto&b:common){bool found=false;for(const auto&x:knownBands[i])if(x==b){found=true;break;}if(found)next.push_back(b);}common=next;}bandStatus=common.empty()?"conflict":"ok";}
 if(bandStatus=="conflict"&&!route->bandOverride){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["bandStatus"]=bandStatus;r["error"]=c_->language=="en"?"Band profiles have no common permission. Enable the explicit override in the signal path first.":"Die Bandprofile besitzen keine gemeinsame Freigabe. Zuerst muss im Signalweg die bewusste Übersteuerung aktiviert werden.";sendJson(r,409);return;}
 if(bandStatus=="conflict"&&route->bandOverride)bandStatus="override";

 for(const auto&cid:routeNodeIds){
  if(cid==c_->controllerId){auto&st=rel_->state();if(rel_->txActive()){error=c_->language=="en"?"Switching blocked: TX active on this controller":"Schalten gesperrt: TX ist auf diesem Steuergerät aktiv";break;}if(st.motorRunning){error=c_->language=="en"?"Switching blocked: timed action is running":"Schalten gesperrt: Eine Zeitaktion läuft";break;}if(st.stormMode){error=c_->language=="en"?"Switching blocked: storm mode is active":"Schalten gesperrt: Gewittermodus ist aktiv";break;}}
  else{String body;if(!remoteBody(cid,body))break;JsonDocument rd;if(deserializeJson(rd,body)){error=c_->language=="en"?"Invalid controller state received":"Ungültiger Steuergerätezustand empfangen";break;}if(rd["txActive"]|false){error=c_->language=="en"?"Switching blocked: TX active on a required controller":"Schalten gesperrt: TX ist auf einem benötigten Steuergerät aktiv";break;}if(rd["motorRunning"]|false){error=c_->language=="en"?"Switching blocked: timed action is running on a required controller":"Schalten gesperrt: Auf einem benötigten Steuergerät läuft eine Zeitaktion";break;}if(rd["stormMode"]|false){error=c_->language=="en"?"Switching blocked: storm mode is active on a required controller":"Schalten gesperrt: Auf einem benötigten Steuergerät ist der Gewittermodus aktiv";break;}}
 }
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["bandStatus"]=bandStatus;r["error"]=error;sendJson(r,409);return;}

 auto remoteActiveFromBody=[&](const String&body,const Req&r)->bool{JsonDocument d;if(deserializeJson(d,body))return false;const char*v=d["activeByGroup"][r.group.c_str()];return v&&String(v)==r.functionId;};
 auto cachedActive=[&](const Req&r)->bool{if(r.controllerId==c_->controllerId)return rel_->isFunctionActive(r.functionId);String body;if(!remoteBody(r.controllerId,body))return false;return remoteActiveFromBody(body,r);};
 auto safeOff=[&](){
  for(const auto&cid:nodeIds){if(cid==c_->controllerId){rel_->emergencyOff();continue;}PeerInfo*p=peerById(cid);if(!p||!p->online)continue;HTTPClient h;h.setTimeout(1200);h.begin("http://"+p->ip+"/api/federation/stop");fed_->addSystemHeaders(h);h.POST("");h.end();}

 };
 int commands=0;bool switchingStarted=false;
 for(const auto&r:reqs){
  if(cachedActive(r))continue;String e;bool ok=false;switchingStarted=true;
  if(r.controllerId==c_->controllerId)ok=rel_->execute(r.functionId,e);else{String resp;ok=fed_->forwardExecute(r.controllerId,r.functionId,resp);if(!ok){JsonDocument q;if(!deserializeJson(q,resp))e=String((const char*)(q["error"]|""));if(e.isEmpty())e=resp;}}
  if(!ok){error=(c_->language=="en"?"Signal path switching failed: ":"Signalweg konnte nicht vollständig geschaltet werden: ")+e;break;}commands++;
 }
 if(!error.isEmpty()){
  if(switchingStarted){safeOff();error+=(c_->language=="en"?". Required controllers were switched to safe OFF.":". Die beteiligten Steuergeräte wurden sicher auf AUS gesetzt.");}store_->addError("ROUTE_ACTIVATE",error);JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["safeOff"]=switchingStarted;r["bandStatus"]=bandStatus;r["error"]=error;sendJson(r,409);return;
 }

 std::vector<String>verifyIds,verifyBodies;auto freshRemote=[&](const String&cid,String&body)->bool{for(size_t i=0;i<verifyIds.size();++i)if(verifyIds[i]==cid){body=verifyBodies[i];return true;}PeerInfo*p=peerById(cid);if(!p||!p->online||!fed_->fetchRemoteSnapshot(*p,body))return false;verifyIds.push_back(cid);verifyBodies.push_back(body);return true;};
 for(const auto&r:reqs){bool active=false;if(r.controllerId==c_->controllerId)active=rel_->isFunctionActive(r.functionId);else{String body;active=freshRemote(r.controllerId,body)&&remoteActiveFromBody(body,r);}if(!active){error=c_->language=="en"?"A controller did not confirm the requested switching state":"Ein Steuergerät hat den angeforderten Schaltzustand nicht bestätigt";break;}}
 if(!error.isEmpty()){safeOff();error+=(c_->language=="en"?". Required controllers were switched to safe OFF.":". Die beteiligten Steuergeräte wurden sicher auf AUS gesetzt.");store_->addError("ROUTE_VERIFY",error);JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["safeOff"]=true;r["bandStatus"]=bandStatus;r["error"]=error;sendJson(r,409);return;}
 JsonDocument out;out["ok"]=true;out["confirmed"]=true;out["routeId"]=route->id;out["label"]=route->label;out["commands"]=commands;out["bandStatus"]=bandStatus;auto ba=out["commonBands"].to<JsonArray>();for(const auto&b:common)ba.add(b);auto na=out["controllers"].to<JsonArray>();for(const auto&cid:nodeIds)na.add(cid);sendJson(out);
}


void WebUi::apiSystemStorm(){
 if(!fed_->isCoordinator()){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Only the active master may change system storm mode":"Nur der aktive Master darf den Anlagen-Gewittermodus ändern";sendJson(r,409);return;}
 JsonDocument in;if(deserializeJson(in,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}bool active=in["active"]|false;
 if(s_->systemStormMode!=active){SharedConfig n=*s_;n.schema=5;n.systemStormMode=active;n.revision=s_->revision+1;n.revisionOrigin=c_->controllerId;String e;if(!store_->saveShared(n,e)){JsonDocument r;r["ok"]=false;r["error"]=e;sendJson(r,409);return;}*s_=n;}
 rel_->setStormMode(active);bool complete=true;int acked=1;JsonDocument out;auto missing=out["missing"].to<JsonArray>();auto peers=fed_->peers();
 for(auto&p:peers){if(!fed_->sameSystem(p)||p.controllerId==c_->controllerId)continue;if(!p.online){complete=false;missing.add(p.name.isEmpty()?p.controllerId:p.name);continue;}HTTPClient h;h.setTimeout(1500);h.begin("http://"+p.ip+"/api/federation/storm");fed_->addSystemHeaders(h);h.addHeader("Content-Type","application/json");JsonDocument q;q["active"]=active;String b;serializeJson(q,b);int code=h.POST(b);String resp=h.getString();h.end();if(code>=200&&code<300)acked++;else{complete=false;missing.add(p.name.isEmpty()?p.controllerId:p.name);}}
 fed_->notifySharedChanged();if(!complete)store_->addError("SYSTEM_STORM_INCOMPLETE",c_->language=="en"?"System storm state was not acknowledged by every controller":"Anlagen-Gewitterzustand wurde nicht von allen Steuergeräten bestätigt");out["ok"]=true;out["active"]=active;out["complete"]=complete;out["acked"]=acked;sendJson(out);
}
void WebUi::apiFederationStorm(){
 if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}JsonDocument in;if(deserializeJson(in,server_.arg("plain"))){server_.send(400,"application/json","{\"ok\":false}");return;}bool active=in["active"]|false;rel_->setStormMode(active);JsonDocument r;r["ok"]=true;r["active"]=rel_->stormActive();sendJson(r);
}
void WebUi::apiFederationGroupOff(){
 if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}JsonDocument in;if(deserializeJson(in,server_.arg("plain"))){server_.send(400,"application/json","{\"ok\":false}");return;}String group=String((const char*)(in["group"]|"")),e;bool ok=rel_->clearGroup(group,e);JsonDocument r;r["ok"]=ok;r["group"]=group;if(!ok)r["error"]=e;sendJson(r,ok?200:409);
}
void WebUi::apiRouteDeactivate(){
 if(!fed_->isCoordinator()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Only the active master may deactivate a signal path":"Nur der aktive Master darf einen Signalweg deaktivieren";sendJson(r,409);return;}
 JsonDocument in;if(deserializeJson(in,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}String routeId=String((const char*)(in["id"]|""));RouteConfig*route=nullptr;for(auto&x:s_->routes)if(x.id==routeId){route=&x;break;}if(!route){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Signal path not found":"Signalweg nicht gefunden";sendJson(r,404);return;}
 struct Req{String controllerId;String functionId;String group;};std::vector<String>cacheIds,cacheBodies;auto peers=fed_->peers();auto peerById=[&](const String&cid)->PeerInfo*{for(auto&p:peers)if(p.controllerId==cid&&fed_->sameSystem(p))return &p;return nullptr;};auto splitKey=[](const String&key,String&cid,String&did)->bool{int p=key.indexOf(':');if(p<=0||p>=(int)key.length()-1)return false;cid=key.substring(0,p);did=key.substring(p+1);return true;};
 auto remoteBody=[&](const String&cid,String&body)->bool{for(size_t i=0;i<cacheIds.size();++i)if(cacheIds[i]==cid){body=cacheBodies[i];return true;}PeerInfo*p=peerById(cid);if(!p||!p->online||!fed_->fetchRemoteSnapshot(*p,body))return false;cacheIds.push_back(cid);cacheBodies.push_back(body);return true;};
 auto addReq=[](std::vector<Req>&out,const Req&r){for(const auto&x:out)if(x.controllerId==r.controllerId&&x.group==r.group){return x.functionId==r.functionId;}out.push_back(r);return true;};
 auto collect=[&](const RouteConfig&rr,std::vector<Req>&out)->bool{out.clear();for(const auto&key:rr.deviceKeys){String cid,did;if(!splitKey(key,cid,did))return false;if(cid==c_->controllerId){const LogicalDeviceConfig*dev=nullptr;for(const auto&d:c_->devices)if(d.enabled&&d.id==did){dev=&d;break;}if(!dev)return false;for(const auto&fid:dev->functionIds){const FunctionConfig*f=nullptr;for(const auto&x:c_->functions)if(x.enabled&&x.visible&&x.id==fid){f=&x;break;}if(f&&f->type==FunctionType::Antenna){Req q{cid,f->id,f->group.isEmpty()?String("ANT"):f->group};if(!addReq(out,q))return false;}}}else{String body;if(!remoteBody(cid,body))return false;JsonDocument rd;if(deserializeJson(rd,body))return false;JsonObjectConst dev;for(JsonObjectConst d:rd["devices"].as<JsonArrayConst>())if(String((const char*)(d["id"]|""))==did){dev=d;break;}if(dev.isNull())return false;for(JsonVariantConst fv:dev["functionIds"].as<JsonArrayConst>()){String fid=String((const char*)(fv|""));for(JsonObjectConst fn:rd["functions"].as<JsonArrayConst>())if(String((const char*)(fn["id"]|""))==fid&&String((const char*)(fn["type"]|""))=="antenna"){Req q{cid,fid,String((const char*)(fn["group"]|"ANT"))};if(!addReq(out,q))return false;break;}}}}return !out.empty();};
 auto active=[&](const std::vector<Req>&rs)->bool{if(rs.empty())return false;for(const auto&r:rs){if(r.controllerId==c_->controllerId){if(!rel_->isFunctionActive(r.functionId))return false;}else{String body;if(!remoteBody(r.controllerId,body))return false;JsonDocument rd;if(deserializeJson(rd,body))return false;const char*v=rd["activeByGroup"][r.group.c_str()];if(!v||String(v)!=r.functionId)return false;}}return true;};
 std::vector<Req>target;if(!collect(*route,target)){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=c_->language=="en"?"Signal path cannot be resolved":"Signalweg kann nicht vollständig aufgelöst werden";sendJson(r,409);return;}std::vector<bool>keep(target.size(),false);for(const auto&other:s_->routes){if(other.id==route->id||other.enabled==false)continue;std::vector<Req>ors;if(!collect(other,ors)||!active(ors))continue;for(size_t i=0;i<target.size();++i)for(const auto&o:ors)if(o.controllerId==target[i].controllerId&&o.group==target[i].group&&o.functionId==target[i].functionId)keep[i]=true;}
 size_t releasable=0;for(bool x:keep)if(!x)releasable++;if(!releasable){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["shared"]=true;r["error"]=c_->language=="en"?"This signal path shares all persistent switching states with another active path and cannot be electrically deactivated separately":"Dieser Signalweg teilt alle dauerhaften Schaltzustände mit einem anderen aktiven Weg und kann elektrisch nicht separat deaktiviert werden";sendJson(r,409);return;}
 String error;std::vector<String>checked;for(size_t i=0;i<target.size()&&error.isEmpty();++i){if(keep[i])continue;bool done=false;for(const auto&x:checked)if(x==target[i].controllerId)done=true;if(done)continue;checked.push_back(target[i].controllerId);if(target[i].controllerId==c_->controllerId){if(rel_->txActive())error=c_->language=="en"?"Deactivation blocked: TX active":"Deaktivieren gesperrt: TX ist aktiv";else if(rel_->state().motorRunning)error=c_->language=="en"?"Deactivation blocked: timed action running":"Deaktivieren gesperrt: Zeitaktion läuft";}else{String body;if(!remoteBody(target[i].controllerId,body)){error=c_->language=="en"?"Controller offline":"Steuergerät nicht erreichbar";break;}JsonDocument rd;if(deserializeJson(rd,body)){error=c_->language=="en"?"Invalid controller state":"Ungültiger Steuergerätezustand";break;}if(rd["txActive"]|false)error=c_->language=="en"?"Deactivation blocked: TX active on required controller":"Deaktivieren gesperrt: TX ist auf einem benötigten Steuergerät aktiv";else if(rd["motorRunning"]|false)error=c_->language=="en"?"Deactivation blocked: timed action running on required controller":"Deaktivieren gesperrt: Auf einem benötigten Steuergerät läuft eine Zeitaktion";}}
 if(!error.isEmpty()){JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}int commands=0;for(size_t i=0;i<target.size();++i){if(keep[i])continue;String e;bool ok=false;if(target[i].controllerId==c_->controllerId)ok=rel_->clearGroup(target[i].group,e);else{PeerInfo*p=peerById(target[i].controllerId);if(p&&p->online){HTTPClient h;h.setTimeout(1500);h.begin("http://"+p->ip+"/api/federation/group/off");fed_->addSystemHeaders(h);h.addHeader("Content-Type","application/json");JsonDocument q;q["group"]=target[i].group;String b;serializeJson(q,b);int code=h.POST(b);String resp=h.getString();h.end();ok=code>=200&&code<300;if(!ok){JsonDocument rd;if(!deserializeJson(rd,resp))e=String((const char*)(rd["error"]|""));}}}if(!ok){error=(c_->language=="en"?"Signal path could not be completely deactivated: ":"Signalweg konnte nicht vollständig deaktiviert werden: ")+e;break;}commands++;}
 if(!error.isEmpty()){store_->addError("ROUTE_DEACTIVATE",error);JsonDocument r;r["ok"]=false;r["confirmed"]=false;r["error"]=error;sendJson(r,409);return;}JsonDocument out;out["ok"]=true;out["confirmed"]=true;out["routeId"]=route->id;out["commands"]=commands;int sharedKept=0;for(bool x:keep)if(x)sharedKept++;out["sharedKept"]=sharedKept;sendJson(out);
}

void WebUi::apiExecute(){
 if(s_->systemStormMode){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"System storm mode is active":"Anlagen-Gewittermodus ist aktiv";sendJson(r,409);return;}
 if(fed_->isUnassigned()){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Controller is not assigned to a system yet":"Steuergerät ist noch keiner Anlage zugeordnet";sendJson(r,409);return;}
 if(!fed_->isCoordinator()&&!federationRequestOk(server_,c_,fed_)){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Switching commands are accepted from the active master only":"Schaltbefehle werden nur vom aktiven Master angenommen";sendJson(r,403);return;}
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",(c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}"));return;}String cid=String((const char*)(d["controllerId"]|c_->controllerId.c_str()));String id=String((const char*)(d["id"]|""));String e;bool ok=false;if(cid==c_->controllerId)ok=rel_->execute(id,e);else {String resp;ok=fed_->forwardExecute(cid,id,resp);server_.send(ok?200:409,"application/json",resp);return;}JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);
}
void WebUi::apiPeers(){
 JsonDocument d;auto a=d.to<JsonArray>();for(auto&p:fed_->peers()){auto o=a.add<JsonObject>();o["controllerId"]=p.controllerId;o["name"]=p.name;o["displayName"]=controllerDisplayName(p.name,p.role,p.failoverPriority);o["callSign"]=p.callSign;o["location"]=p.location;o["firmware"]=p.firmware;o["boardProfile"]=p.boardProfile;o["failoverPriority"]=p.failoverPriority;o["ip"]=p.ip;o["online"]=p.online;o["rev"]=p.sharedRevision;o["role"]=p.role;o["systemName"]=p.systemName;o["admissionMode"]=p.admissionMode;o["coordinator"]=p.coordinator;o["relation"]=p.role=="unassigned"?"unassigned":(fed_->sameSystem(p)?"same":"foreign");}sendJson(d);
}
void WebUi::apiCombined(){
 JsonDocument d;d["firmwareVersion"]=ANTCTRL_VERSION;d["language"]=c_->language;d["localTime"]=time_->local();d["utcTime"]=time_->utc();d["showLocal"]=c_->time.showLocal;d["showUtc"]=c_->time.showUtc;d["showDate"]=c_->time.showDate;
 auto sys=d["system"].to<JsonObject>();sys["role"]=c_->federation.role;sys["assigned"]=!fed_->isUnassigned();sys["systemName"]=c_->federation.systemName;sys["systemHost"]=fed_->systemHost();sys["systemUrl"]=fed_->systemHost().isEmpty()?String():String("http://")+fed_->systemHost()+".local/";sys["coordinator"]=fed_->isCoordinator();sys["temporaryCoordinator"]=fed_->isTemporaryCoordinator();sys["reclaiming"]=fed_->reclaiming();sys["permanentMasterOnline"]=fed_->permanentMasterOnline();sys["masterOnline"]=fed_->masterOnline();sys["masterIp"]=fed_->masterIp();sys["admissionMode"]=c_->federation.admissionMode;sys["stormMode"]=s_->systemStormMode;
 JsonArray a=d["controllers"].to<JsonArray>();
 auto so=a.add<JsonObject>();so["controllerId"]=c_->controllerId;so["name"]=c_->identity.deviceName;so["displayName"]=controllerDisplayName(c_->identity.deviceName,c_->federation.role,c_->federation.failoverPriority);so["callSign"]=c_->identity.callSign;so["description"]=c_->identity.description;so["location"]=c_->identity.location;so["ip"]=WiFi.localIP().toString();so["self"]=true;so["online"]=true;so["federationEnabled"]=c_->federation.enabled;so["firmware"]=ANTCTRL_VERSION;so["boardProfile"]=c_->boardProfile;so["role"]=c_->federation.role;so["coordinator"]=fed_->isCoordinator();so["txActive"]=rel_->txActive();auto&st=rel_->state();so["activeAntenna"]=st.activeAntennaId;auto sabg=so["activeByGroup"].to<JsonObject>();for(const auto&sel:st.activeSelections)sabg[sel.group]=sel.functionId;so["polarization"]=st.polarization;so["motorRunning"]=st.motorRunning;so["motorFunctionId"]=st.motorFunctionId;so["motorRemainingMs"]=rel_->motorRemainingMs();so["stormMode"]=st.stormMode;auto fs=so["functions"].to<JsonArray>();for(auto&x:c_->functions)if(x.enabled&&x.visible){auto q=fs.add<JsonObject>();q["id"]=x.id;q["label"]=x.label;q["type"]=x.type==FunctionType::Timed?"timed":(x.type==FunctionType::Storm?"storm":(x.type==FunctionType::Toggle?"toggle":"antenna"));q["requires"]=x.requiresFunctionId;q["group"]=x.group;q["stateToken"]=x.stateToken;q["durationMs"]=x.durationMs;bool configured=x.type==FunctionType::Storm;for(const auto&r:c_->relays)if(r.enabled&&r.id==x.relayId)configured=true;q["configured"]=configured;}auto ds=so["devices"].to<JsonArray>();for(const auto&x:c_->devices)if(x.enabled)deviceJson(ds.add<JsonObject>(),x);
 for(auto&p:fed_->peers())if(fed_->sameSystem(p)){auto o=a.add<JsonObject>();o["controllerId"]=p.controllerId;o["name"]=p.name;o["displayName"]=controllerDisplayName(p.name,p.role,p.failoverPriority);o["callSign"]=p.callSign;o["location"]=p.location;o["ip"]=p.ip;o["self"]=false;o["firmware"]=p.firmware;o["boardProfile"]=p.boardProfile;o["role"]=p.role;o["coordinator"]=p.coordinator;o["online"]=p.online;if(p.online){String js;if(fed_->fetchRemoteSnapshot(p,js,350)){JsonDocument rd;if(!deserializeJson(rd,js)){o["ip"]=String((const char*)(rd["ip"]|p.ip.c_str()));o["activeAntenna"]=rd["activeAntenna"];o["activeByGroup"].set(rd["activeByGroup"]);o["polarization"]=rd["polarization"];o["motorRunning"]=rd["motorRunning"];o["motorFunctionId"]=rd["motorFunctionId"];o["motorRemainingMs"]=rd["motorRemainingMs"];o["txActive"]=rd["txActive"];o["stormMode"]=rd["stormMode"];o["location"]=rd["location"];o["firmware"]=rd["firmwareVersion"];o["functions"].set(rd["functions"]);o["devices"].set(rd["devices"]);}}}}
 auto discovered=d["discovered"].to<JsonArray>();for(auto&p:fed_->peers()){auto o=discovered.add<JsonObject>();o["controllerId"]=p.controllerId;o["name"]=p.name;o["displayName"]=controllerDisplayName(p.name,p.role,p.failoverPriority);o["callSign"]=p.callSign;o["location"]=p.location;o["ip"]=p.ip;o["firmware"]=p.firmware;o["boardProfile"]=p.boardProfile;o["online"]=p.online;o["role"]=p.role;o["systemName"]=p.systemName;o["admissionMode"]=p.admissionMode;o["relation"]=p.role=="unassigned"?"unassigned":(fed_->sameSystem(p)?"same":"foreign");}
 auto direct=d["provisioning"].to<JsonArray>();if(wifiMgr_&&fed_->isMaster()){for(const auto&x:wifiMgr_->provisioningCandidates()){if(x.recovery)continue;auto o=direct.add<JsonObject>();o["controllerId"]=x.controllerId;o["boardProfile"]=x.boardProfile;o["online"]=x.online;o["authorizedMode"]=x.authorizedMode;}}
 JsonDocument sh;sharedJson(sh,*s_);d["shared"].set(sh);
 // The clock and motor countdown change continuously but do not invalidate the
 // rest of the snapshot. Temporarily omit them without duplicating the document.
 String localTime=d["localTime"].as<String>(),utcTime=d["utcTime"].as<String>();d.remove("localTime");d.remove("utcTime");
 JsonArray controllers=d["controllers"].as<JsonArray>();std::vector<uint32_t> remaining;if(!controllers.isNull())for(JsonObject controller:controllers){remaining.push_back(controller["motorRemainingMs"]|0u);controller.remove("motorRemainingMs");}
 String canonical;serializeJson(d,canonical);uint32_t hashA=2166136261u,hashB=0x9e3779b9u;for(size_t i=0;i<canonical.length();++i){uint8_t byte=(uint8_t)canonical[i];hashA=(hashA^byte)*16777619u;hashB^=byte+0x9e3779b9u+(hashB<<6)+(hashB>>2);}
 d["localTime"]=localTime;d["utcTime"]=utcTime;if(!controllers.isNull()){size_t i=0;for(JsonObject controller:controllers)controller["motorRemainingMs"]=remaining[i++];}
 char tag[20];snprintf(tag,sizeof(tag),"\"%08lx%08lx\"",(unsigned long)hashA,(unsigned long)hashB);server_.sendHeader("ETag",tag);server_.sendHeader("Cache-Control","no-cache");
 if(server_.header("If-None-Match")==tag){server_.send(304,"application/json; charset=utf-8","");return;}
 sendJson(d);
}

void WebUi::apiErrors(){JsonDocument d;auto a=d.to<JsonArray>();for(auto&e:store_->errors()){auto o=a.add<JsonObject>();o["ts"]=e.epoch;o["code"]=e.code;o["text"]=e.text;}sendJson(d);}
void WebUi::apiNews(){
 JsonDocument d;if(!fed_->isCoordinator()){d["enabled"]=false;d["fetching"]=false;d["centralService"]=false;sendJson(d);return;}NewsSnapshot snapshot=news_->snapshot();d["enabled"]=snapshot.enabled;d["fetching"]=snapshot.fetching;d["lastError"]=snapshot.lastError;d["lastHttpCode"]=snapshot.lastHttpCode;d["lastSuccessEpoch"]=snapshot.lastSuccessEpoch;
 auto ss=d["sources"].to<JsonArray>();for(auto&x:snapshot.sources){auto o=ss.add<JsonObject>();o["id"]=x.id;o["name"]=x.name;o["ok"]=x.ok;o["stale"]=x.stale;o["httpCode"]=x.httpCode;o["itemCount"]=x.itemCount;o["error"]=x.error;}
 auto a=d["items"].to<JsonArray>();for(auto&x:snapshot.items){auto o=a.add<JsonObject>();o["title"]=x.title;o["summary"]=x.summary;o["link"]=x.link;o["sourceId"]=x.sourceId;o["source"]=x.source;o["date"]=x.date;}
 sendJson(d);
}
void WebUi::apiWeather(){
 JsonDocument d;if(!fed_->isCoordinator()){d["enabled"]=false;d["valid"]=false;d["fetching"]=false;d["centralService"]=false;sendJson(d);return;}
 if(!weather_){d["enabled"]=false;d["valid"]=false;d["fetching"]=false;sendJson(d);return;}
 WeatherInfo w=weather_->info();
 d["enabled"]=w.enabled;d["valid"]=w.valid;d["stale"]=w.valid&&!w.error.isEmpty();d["fetching"]=w.fetching;d["lightningEnabled"]=c_->lightning.enabled;d["lightningWarningKm"]=c_->lightning.warningKm;d["lightningDangerKm"]=c_->lightning.dangerKm;d["lightningBoxKm"]=c_->lightning.boxKm;d["postalCode"]=w.postalCode;d["place"]=w.place;d["latitude"]=w.latitude;d["longitude"]=w.longitude;d["error"]=w.error;d["temperature"]=w.temperature;d["apparentTemperature"]=w.apparentTemperature;d["humidity"]=w.humidity;d["precipitation"]=w.precipitation;d["weatherCode"]=w.weatherCode;d["windSpeed"]=w.windSpeed;d["windDirection"]=w.windDirection;d["windGusts"]=w.windGusts;d["todayMax"]=w.todayMax;d["todayMin"]=w.todayMin;d["todayPrecipitationProbability"]=w.todayPrecipitationProbability;d["todayWeatherCode"]=w.todayWeatherCode;d["lastSuccessEpoch"]=w.lastSuccessEpoch;
 sendJson(d);
}
void WebUi::setupSaved(){
 JsonDocument d;JsonArray a=d.to<JsonArray>();size_t idx=0;
 for(const auto&w:store_->wifiNetworks()){auto o=a.add<JsonObject>();o["index"]=idx++;o["ssid"]=w.ssid;o["hasPassword"]=!w.password.isEmpty();}
 sendJson(d);
}
void WebUi::setupScan(){
 int n=wifiMgr_?wifiMgr_->scanNetworks():WiFi.scanNetworks(false,true);JsonDocument d;JsonArray a=d.to<JsonArray>();
 if(n>0){for(int i=0;i<n;++i){String ssid=WiFi.SSID(i);if(ssid.isEmpty())continue;bool duplicate=false;for(JsonObject x:a){if(String((const char*)(x["ssid"]|""))==ssid){duplicate=true;break;}}if(duplicate)continue;auto o=a.add<JsonObject>();o["ssid"]=ssid;o["rssi"]=WiFi.RSSI(i);o["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;}}
 WiFi.scanDelete();sendJson(d);
}
void WebUi::setupAdd(){
 JsonDocument d;String ssid=server_.arg("ssid"),pass=server_.arg("pass"),pageLanguage=c_->language;
 if(server_.hasArg("plain")&&!server_.arg("plain").isEmpty()&&!deserializeJson(d,server_.arg("plain"))){ssid=String((const char*)(d["ssid"]|""));pass=String((const char*)(d["password"]|""));pageLanguage=String((const char*)(d["language"]|pageLanguage.c_str()));}
 String e;if(!wifiMgr_||!wifiMgr_->testAndAddNetwork(ssid,pass,e,pageLanguage=="en")){JsonDocument r;r["ok"]=false;r["error"]=e;sendJson(r,409);return;}
 JsonDocument r;r["ok"]=true;r["ssid"]=ssid;r["restart"]=true;sendJson(r);delay(800);ESP.restart();
}
void WebUi::setupDelete(){
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"error\":\"Invalid JSON\"}":"{\"error\":\"JSON ungültig\"}");return;}String pageLanguage=String((const char*)(d["language"]|c_->language.c_str()));size_t index=d["index"]|999U;String e;bool ok=store_->deleteWifi(index,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=pageLanguage=="en"?"Wi-Fi settings could not be changed":e;sendJson(r,ok?200:409);
}
void WebUi::setupMove(){
 JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"error\":\"Invalid JSON\"}":"{\"error\":\"JSON ungültig\"}");return;}String pageLanguage=String((const char*)(d["language"]|c_->language.c_str()));size_t index=d["index"]|999U;int dir=d["direction"]|0;String e;bool ok=store_->moveWifi(index,dir,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=pageLanguage=="en"?"Wi-Fi settings could not be changed":e;sendJson(r,ok?200:409);
}
void WebUi::apiEnterSetupMode(){
 if(!wifiMgr_){server_.send(500,"application/json","{\"ok\":false}");return;}wifiMgr_->requestSetupMode();server_.send(200,"application/json","{\"ok\":true,\"restart\":true}");delay(350);ESP.restart();
}
void WebUi::apiWifi(){
 setupAdd();
}
void WebUi::apiPullShared(){
 if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}
 if(!fed_->isFollower()){server_.send(409,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Only a follower pulls shared configuration\"}":"{\"ok\":false,\"error\":\"Nur ein Follower übernimmt die gemeinsame Anlagenkonfiguration\"}");return;}
 String ip=server_.client().remoteIP().toString();HTTPClient h;h.setTimeout(1800);h.begin("http://"+ip+"/api/shared?internal=1");fed_->addSystemHeaders(h);int code=h.GET();
 if(code==200){String body=h.getString();JsonDocument x;if(!deserializeJson(x,body)){if(!x["layout"].is<JsonArray>()||!x["displayGroups"].is<JsonArray>()||!x["routes"].is<JsonArray>()){JsonDocument r;r["ok"]=false;r["error"]="Unvollständige gemeinsame Konfiguration abgewiesen";h.end();sendJson(r,409);return;}SharedConfig n;int remoteSchema=x["schema"]|1;n.schema=5;n.systemStormMode=remoteSchema>=4?(x["systemStormMode"]|false):false;n.revision=x["revision"]|1;n.revisionOrigin=String((const char*)(x["revisionOrigin"]|""));n.systemTitle=String((const char*)(x["systemTitle"]|"Antennensteuerung"));if(remoteSchema>=5){JsonObject sec=x["security"].as<JsonObject>();n.security.adminAuthEnabled=sec["adminAuthEnabled"]|false;n.security.adminUser=String((const char*)(sec["adminUser"]|"admin"));n.security.adminPassword=String((const char*)(sec["adminPassword"]|""));}uiFrom(x["ui"],n.ui);if(remoteSchema>=2){for(JsonObject q:x["layout"].as<JsonArray>()){LayoutItem z;z.key=String((const char*)(q["key"]|""));z.w=constrain((int)(q["w"]|3),1,12);z.h=constrain((int)(q["h"]|1),1,6);z.x=constrain((int)(q["x"]|0),0,12-z.w);z.y=constrain((int)(q["y"]|0),0,6-z.h);z.fontPx=constrain((int)(q["fontPx"]|20),10,48);z.visible=q["visible"]|true;n.layout.push_back(z);}}if(remoteSchema>=3){for(JsonObjectConst q:x["displayGroups"].as<JsonArrayConst>()){DisplayGroupConfig g;displayGroupFromJson(q,g);if(!g.id.isEmpty()&&!g.title.isEmpty())n.displayGroups.push_back(g);}for(JsonObjectConst q:x["routes"].as<JsonArrayConst>()){RouteConfig r;routeFromJson(q,r);if(!r.id.isEmpty())n.routes.push_back(r);}}if(n.displayGroups.empty())n.displayGroups={{"radios","Funkgeräte",10},{"middle","PA / Tuner / Filter",20},{"antennas","Antennen",30}};String e;if(store_->saveShared(n,e,true)){*s_=n;rel_->setStormMode(n.systemStormMode);}else{JsonDocument r;r["ok"]=false;r["error"]=e;h.end();sendJson(r,409);return;}}}
 h.end();server_.send(code==200?204:502,"application/json",code==200?"":"{\"ok\":false}");
}
void WebUi::otaPage(){
  updateBackupDownloaded=false;
  if(!c_->features.ota){server_.send(404,"text/plain",c_->language=="en"?"OTA disabled":"OTA deaktiviert");return;}
  bool en=c_->language=="en";
  String page="<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>"+String(en?"Antenna Controller Update":"Antennensteuerung Aktualisierung")+"</title><style>body{font-family:system-ui;background:#101216;color:#eef2f6;max-width:820px;margin:32px auto;padding:20px}section{background:#1a1e25;border:1px solid #343b46;padding:18px;border-radius:12px;margin:16px 0}.warn{border-left:4px solid #d69a36}button,a.button{font-size:1rem;padding:12px 16px}button{border:0;border-radius:6px;background:#2874b8;color:white;font-weight:700;cursor:pointer}button:disabled{opacity:.5;cursor:wait}a{color:#77c9f4}.ok{color:#8ed9a5}#status{min-height:1.5em}</style><h1>"+String(en?"Program / update":"Programm / Update")+" · v"+String(ANTCTRL_VERSION)+"</h1>";
  page+="<section class=warn><h2>"+String(en?"Update this controller":"Dieses Steuergerät aktualisieren")+"</h2><p>"+String(en?"One click downloads this controller's configuration and Wi-Fi backup, saves it in your browser's downloads, then installs the latest stable firmware from GitHub. Settings are preserved.":"Ein Klick lädt die Konfigurations- und WLAN-Sicherung dieses Steuergeräts herunter und speichert sie bei den Browser-Downloads. Danach installiert der ESP die neueste stabile Firmware von GitHub. Die Einstellungen bleiben erhalten.")+"</p><p><b>"+String(en?"Cluster: only this controller is updated and backed up. Update each other controller separately.":"Verbund: Nur dieses Steuergerät wird aktualisiert und gesichert. Weitere Steuergeräte müssen einzeln aktualisiert werden.")+"</b> "+String(en?"The backup filename contains the controller ID and IP.":"Controller-ID und IP stehen im Dateinamen der Sicherung.")+"</p><button id=installLatest>"+String(en?"Download backup and install latest firmware":"Sicherung laden und neueste Firmware installieren")+"</button><p id=status role=status aria-live=polite></p></section>";
  page+="<section><h2>"+String(en?"Previous firmware":"Vorherige Firmware starten")+"</h2><p>"+String(en?"Rollback is available only when the ESP still has a valid previous firmware partition. A safety backup is created first.":"Zurückschalten ist nur möglich, wenn noch eine gültige vorherige Firmwarepartition vorhanden ist. Zuerst wird eine interne Sicherung erstellt.")+"</p><button id=rollbackSubmit>"+String(en?"Start previous firmware":"Vorherige Firmware starten")+"</button></section>";
  page+="<script>const b=document.getElementById('installLatest'),s=document.getElementById('status');b.onclick=async()=>{b.disabled=true;s.textContent='"+String(en?"Downloading backup…":"Sicherung wird heruntergeladen …")+"';try{const backup=await fetch('/api/update-backup',{cache:'no-store'});if(!backup.ok)throw Error(await backup.text());const blob=await backup.blob(),a=document.createElement('a'),url=URL.createObjectURL(blob);a.href=url;a.download=(backup.headers.get('Content-Disposition')||'').match(/filename=\"?([^\";]+)\"?/i)?.[1]||'Antennensteuerung-Sicherung.json';document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),30000);s.textContent='"+String(en?"Installing the latest firmware…":"Neueste Firmware wird installiert …")+"';const r=await fetch('/api/update/install-latest',{method:'POST',cache:'no-store'}),j=await r.json();if(!r.ok||!j.ok)throw Error(j.error||'Update failed');s.className='ok';s.textContent='"+String(en?"Update complete. The controller is restarting; this page will reconnect automatically.":"Update abgeschlossen. Das Steuergerät startet neu; diese Seite verbindet sich automatisch wieder.")+"';setTimeout(()=>location.reload(),12000)}catch(e){s.textContent=e.message;b.disabled=false}};document.getElementById('rollbackSubmit').onclick=async()=>{if(!confirm('"+String(en?"Start the previous firmware?":"Vorherige Firmware wirklich starten?")+"'))return;try{const r=await fetch('/rollback',{method:'POST'});const t=await r.text();if(!r.ok)throw Error(t);s.textContent=t}catch(e){s.textContent=e.message}};</script>";
  server_.send(200,"text/html; charset=utf-8",page);
}
void WebUi::otaInstallLatest(){
  if(!c_->features.ota){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"OTA disabled":"OTA deaktiviert";sendJson(r,404);return;}
  if(!updateBackupDownloaded){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Download the configuration backup before updating":"Bitte zuerst die Konfigurationssicherung herunterladen";sendJson(r,409);return;}
  updateBackupDownloaded=false;String error;if(!ensureFreshUpdateSafetyBackup(store_,error)){JsonDocument r;r["ok"]=false;r["error"]=(c_->language=="en"?"Safety backup failed. Update refused: ":"Sicherheitskopie fehlgeschlagen. Aktualisierung verweigert: ")+error;sendJson(r,409);return;}
  if(WiFi.status()!=WL_CONNECTED){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"No internet connection":"Keine Internetverbindung";sendJson(r,503);return;}
  WiFiClientSecure tls;tls.setCACertBundle(GITHUB_TRUST_BUNDLE);HTTPClient h;h.setConnectTimeout(12000);h.setTimeout(30000);h.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if(!h.begin(tls,"https://github.com/zentrog/ESP32-Antennenschalter-Produkt/releases/latest/download/firmware-esp32dev.bin")){JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Could not prepare the GitHub download":"GitHub-Download konnte nicht vorbereitet werden";sendJson(r,502);return;}
  int code=h.GET();int32_t expected=h.getSize();if(code!=200||expected<512U*1024U||expected>3U*1024U*1024U){h.end();JsonDocument r;r["ok"]=false;r["error"]=(c_->language=="en"?"GitHub firmware download failed (HTTP ":"GitHub-Firmwaredownload fehlgeschlagen (HTTP ")+String(code)+")";sendJson(r,502);return;}
  if(!Update.begin((size_t)expected)){h.end();JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Firmware does not fit in the OTA partition":"Firmware passt nicht in die OTA-Partition";sendJson(r,507);return;}
  size_t written=Update.writeStream(*h.getStreamPtr());h.end();
  if(written!=(size_t)expected||!Update.end(true)){Update.abort();JsonDocument r;r["ok"]=false;r["error"]=c_->language=="en"?"Download was incomplete or firmware validation failed":"Download unvollständig oder Firmware-Prüfung fehlgeschlagen";sendJson(r,500);return;}
  JsonDocument r;r["ok"]=true;r["bytes"]=written;r["firmware"]=ANTCTRL_VERSION;server_.sendHeader("Connection","close");sendJson(r);delay(500);ESP.restart();
}
void WebUi::otaRollback(){
  if(!updateBackupDownloaded){server_.send(409,"text/plain; charset=utf-8",c_->language=="en"?"Download the configuration backup before rollback":"Bitte zuerst die Konfigurationssicherung herunterladen");return;}
  if(!c_->features.ota){server_.send(404,"text/plain",c_->language=="en"?"OTA disabled":"OTA deaktiviert");return;}
  String e;if(!createUpdateSafetyBackup(store_,e)){server_.send(409,"text/plain; charset=utf-8",(c_->language=="en"?"Safety backup failed. Rollback refused: ":"Sicherheitskopie fehlgeschlagen. Zurückschalten verweigert: ")+e);return;}
  if(!Update.canRollBack()){server_.send(409,"text/plain",c_->language=="en"?"No valid previous firmware partition available":"Keine gültige vorherige Programmpartition verfügbar");return;}
  if(!Update.rollBack()){server_.send(500,"text/plain",c_->language=="en"?"Previous firmware could not be activated":"Vorherige Programmversion konnte nicht aktiviert werden");return;}
  server_.send(200,"text/plain",c_->language=="en"?"Safety backup created. Previous firmware activated, restarting":"Sicherheitskopie erstellt. Vorherige Programmversion aktiviert, Neustart");delay(400);ESP.restart();
}
void WebUi::apiStop(){
 rel_->emergencyOff();JsonDocument r;r["scope"]=fed_->isCoordinator()?"system":"local";r["local"]=true;bool complete=true;auto nodes=r["nodes"].to<JsonArray>();
 if(fed_->isCoordinator())for(auto&p:fed_->peers())if(fed_->sameSystem(p)){auto n=nodes.add<JsonObject>();n["name"]=p.name;n["ip"]=p.ip;if(!p.online){n["ok"]=false;n["error"]=c_->language=="en"?"offline":"nicht erreichbar";complete=false;continue;}HTTPClient h;h.setTimeout(1500);h.begin("http://"+p.ip+"/api/federation/stop");fed_->addSystemHeaders(h);int code=h.POST("");String body=h.getString();h.end();bool ok=code>=200&&code<300;n["ok"]=ok;if(!ok){n["error"]="HTTP "+String(code);complete=false;}}
 r["ok"]=complete;r["complete"]=complete;sendJson(r);
}
void WebUi::apiFederationStop(){if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"application/json","{\"ok\":false}");return;}rel_->emergencyOff();JsonDocument r;r["ok"]=true;r["local"]=true;sendJson(r);}
void WebUi::apiSystemAdmission(){JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400);return;}String mode=String((const char*)(d["mode"]|"")),e;bool ok=fed_->setAdmissionMode(mode,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;else r["mode"]=mode;sendJson(r,ok?200:409);}
void WebUi::apiProvisionAuthorize(){JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400);return;}String id=String((const char*)(d["controllerId"]|"")),mode=String((const char*)(d["mode"]|"")),e;bool ok=wifiMgr_&&wifiMgr_->authorizeProvisioning(id,mode,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;else{r["controllerId"]=id;r["mode"]=mode;}sendJson(r,ok?200:409);}
void WebUi::apiSystemAdopt(){JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400);return;}String id=String((const char*)(d["controllerId"]|"")),e;bool ok=fed_->adoptPeer(id,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);}
void WebUi::apiSystemMakeMaster(){JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400);return;}String id=String((const char*)(d["controllerId"]|"")),e;bool ok=fed_->makePeerMaster(id,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);}
void WebUi::apiFederationAdopt(){if(!onboardingRequestOk(server_)){server_.send(403,"application/json","{\"ok\":false}");return;}JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400);return;}String sid=String((const char*)(d["systemId"]|"")),name=String((const char*)(d["systemName"]|"Antennenanlage")),mid=String((const char*)(d["masterId"]|"")),e;uint16_t priority=d["failoverPriority"]|100;bool ok=fed_->acceptAssignment(sid,name,mid,priority,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);}
void WebUi::apiFederationMakeMaster(){if(!onboardingRequestOk(server_)){server_.send(403,"application/json","{\"ok\":false}");return;}String e;bool ok=fed_->becomeOwnMaster(e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);}
void WebUi::apiRestart(){server_.send(200,"application/json","{\"ok\":true}");delay(350);ESP.restart();}

void WebUi::apiControllerConfigGet(){
  String id=server_.arg("id");if(id.isEmpty()||id==c_->controllerId){apiConfigGet();return;}
  for(auto&p:fed_->peers())if(p.controllerId==id&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/config"+(server_.arg("force")=="1"?"?force=1":""));fed_->addSystemHeaders(h);
    int code=h.GET();String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiControllerConfigPut(){
  String id=server_.arg("id");if(id.isEmpty()||id==c_->controllerId){apiConfigPut();return;}
  for(auto&p:fed_->peers())if(p.controllerId==id&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/config"+(server_.arg("force")=="1"?"?force=1":""));fed_->addSystemHeaders(h);h.addHeader("Content-Type","application/json");
    int code=h.PUT(server_.arg("plain"));String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiFederationConfigGet(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403);return;}
  JsonDocument d;cfgJson(d,*c_,true);sendJson(d);
}
void WebUi::apiFederationConfigPut(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403);return;}
  apiConfigPut();
}

void WebUi::apiTestRelay(){
  JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",(c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}"));return;}
  String id=String((const char*)(d["relayId"]|"")),e;bool ok=rel_->testRelay(id,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);
}
void WebUi::apiTestGpio(){
  JsonDocument d;if(deserializeJson(d,server_.arg("plain"))){server_.send(400,"application/json",c_->language=="en"?"{\"ok\":false,\"error\":\"Invalid JSON\"}":"{\"ok\":false,\"error\":\"JSON ungültig\"}");return;}
  int gpio=d["gpio"]|-1;String e;bool ok=rel_->testGpio(gpio,e);JsonDocument r;r["ok"]=ok;if(!ok)r["error"]=e;sendJson(r,ok?200:409);
}
void WebUi::apiFactoryReset(){
  rel_->emergencyOff();store_->factoryReset();server_.send(200,"application/json","{\"ok\":true}");delay(400);ESP.restart();
}

void WebUi::apiControllerTestRelay(){
  String cid=server_.arg("id");
  if(cid.isEmpty()||cid==c_->controllerId){apiTestRelay();return;}
  for(auto&p:fed_->peers())if(p.controllerId==cid&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/test-relay");fed_->addSystemHeaders(h);h.addHeader("Content-Type","application/json");
    int code=h.POST(server_.arg("plain"));String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiFederationTestRelay(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}
  apiTestRelay();
}
void WebUi::apiControllerTestGpio(){
  String cid=server_.arg("id");if(cid.isEmpty()||cid==c_->controllerId){apiTestGpio();return;}
  for(auto&p:fed_->peers())if(p.controllerId==cid&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/test-gpio");fed_->addSystemHeaders(h);h.addHeader("Content-Type","application/json");
    int code=h.POST(server_.arg("plain"));String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiFederationTestGpio(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}
  apiTestGpio();
}
void WebUi::apiControllerRestart(){
  String cid=server_.arg("id");
  if(cid.isEmpty()||cid==c_->controllerId){apiRestart();return;}
  for(auto&p:fed_->peers())if(p.controllerId==cid&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/restart");fed_->addSystemHeaders(h);int code=h.POST("");String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiFederationRestart(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}
  apiRestart();
}
void WebUi::apiControllerFactoryReset(){
  String cid=server_.arg("id");
  if(cid.isEmpty()||cid==c_->controllerId){apiFactoryReset();return;}
  for(auto&p:fed_->peers())if(p.controllerId==cid&&p.online&&fed_->sameSystem(p)){
    HTTPClient h;h.begin("http://"+p.ip+"/api/federation/factory-reset");fed_->addSystemHeaders(h);int code=h.POST("");String body=h.getString();h.end();server_.send(code,"application/json",body);return;
  }
  server_.send(404,"application/json",c_->language=="en"?"{\"error\":\"Controller offline/unknown\"}":"{\"error\":\"Steuergerät nicht erreichbar oder unbekannt\"}");
}
void WebUi::apiFederationFactoryReset(){
  if(!federationRequestOk(server_,c_,fed_)){server_.send(403,"text/plain",c_->language=="en"?"Forbidden":"Zugriff verweigert");return;}
  apiFactoryReset();
}

