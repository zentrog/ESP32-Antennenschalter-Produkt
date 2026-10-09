#pragma once
#include "Model.h"
#include "Storage.h"
#include "RelayEngine.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct PeerInfo {
  String controllerId,name,callSign,location,firmware,boardProfile,ip;
  String systemId,systemName,role,permanentMasterId,admissionMode;
  uint32_t lastSeen=0,sharedRevision=0;
  String revisionOrigin;
  uint16_t failoverPriority=100;
  bool coordinator=false;
  bool busy=false;
  bool online=false;
};
struct NewsItem { String title,summary,link,sourceId,source,date; };
struct NewsSourceStatus { String id,name,error; bool ok=false,stale=false; int httpCode=0,itemCount=0; };
struct NewsSnapshot {
  std::vector<NewsItem> items;
  std::vector<NewsSourceStatus> sources;
  String lastError;
  int lastHttpCode=0;
  uint32_t lastSuccessEpoch=0;
  bool enabled=false;
  bool fetching=false;
};

struct WeatherInfo {
  bool enabled=false;
  bool valid=false;
  bool fetching=false;
  String postalCode;
  String place;
  String error;
  uint32_t lastSuccessEpoch=0;
  float temperature=0;
  float apparentTemperature=0;
  int humidity=0;
  float precipitation=0;
  int weatherCode=-1;
  float windSpeed=0;
  int windDirection=0;
  float windGusts=0;
  float todayMax=0;
  float todayMin=0;
  float latitude=0;
  float longitude=0;
  int todayPrecipitationProbability=0;
  int todayWeatherCode=-1;
};

class TimeService {
 public:void begin(const TimeConfig&);bool synced()const;String local()const;String utc()const;uint32_t epoch()const;
};

class MqttService {
 public:
  void begin(LocalConfig*,RelayEngine*);void loop();void publishState();void configChanged();
 private:
  LocalConfig* c_=nullptr;RelayEngine* r_=nullptr;WiFiClient net_;PubSubClient mqtt_{net_};uint32_t lastTry_=0,lastPub_=0;
  static MqttService* self_;static void thunk(char*,byte*,unsigned int);void onMessage(char*,byte*,unsigned int);String base()const;
};

class FederationService {
 public:
  void begin(LocalConfig*,SharedConfig*,Storage*,RelayEngine*);void loop();void configChanged();std::vector<PeerInfo> peers()const;
  bool forwardExecute(const String& controllerId,const String& functionId,String& response);
  bool fetchRemoteSnapshot(const PeerInfo&,String& json,uint16_t timeoutMs=1200);
  void notifySharedChanged();
  bool isMaster()const;bool isFollower()const;bool isUnassigned()const;bool isCoordinator()const;
  bool sameSystem(const PeerInfo&)const;bool masterOnline()const;String masterIp()const;
  bool permanentMasterOnline()const;bool isTemporaryCoordinator()const{return temporaryCoordinator_;}bool reclaiming()const{return reclaiming_;}
  String activeCoordinatorId()const;String systemHost()const;bool authorizedCoordinator(const String& controllerId)const;
  bool setAdmissionMode(const String& mode,String& error);
  bool adoptPeer(const String& controllerId,String& error);
  bool makePeerMaster(const String& controllerId,String& error);
  bool acceptAssignment(const String& systemId,const String& systemName,const String& masterId,uint16_t failoverPriority,String& error);
  bool becomeOwnMaster(String& error);
  void addSystemHeaders(HTTPClient& h)const;
 private:
  LocalConfig* c_=nullptr;SharedConfig*s_=nullptr;Storage*store_=nullptr;RelayEngine*relay_=nullptr;WiFiUDP udp_;std::vector<PeerInfo> peers_;uint32_t lastHello_=0,lastSweep_=0,startedAt_=0,lastBootstrap_=0,lastAutoAdopt_=0;
  bool temporaryCoordinator_=false,masterReady_=false,reclaiming_=false,networkWasConnected_=false;uint32_t masterMissingSince_=0,reclaimStableSince_=0;
  void sendHello();void receive();void upsert(const PeerInfo&);void synchronize(PeerInfo&);void bootstrap();void updateFailover();
  bool promoteSelfToMaster(String& error);bool sendAdopt(PeerInfo&,bool ownMaster,String& error);
  bool networkWitness()const;bool localBusy()const;bool systemBusy();bool confirmSystemIdle();bool pullSharedFrom(PeerInfo&);void pushSharedToFollowers();
  bool betterFallback(uint16_t aPriority,const String&aId,uint16_t bPriority,const String&bId)const;String electedFallbackId()const;uint16_t allocateFollowerPriority();
};

class NewsService {
 public:
  void begin(LocalConfig*);void loop();void configChanged();void requestRefresh();
  NewsSnapshot snapshot()const;
 private:
  LocalConfig*c_=nullptr;SemaphoreHandle_t mutex_=nullptr;NewsConfig config_;String language_;std::vector<NewsItem>items_;std::vector<NewsSourceStatus>sources_;uint32_t lastFetch_=0,lastSuccessEpoch_=0,generation_=0;int lastHttpCode_=0;bool fetching_=false,workerRunning_=false;String lastError_;
  static void workerThunk(void*);
  void fetchAll(const NewsConfig&,const String&,uint32_t);
  bool fetchFeed(const NewsFeed&,const String&,std::vector<NewsItem>&,int,String&,int&);
};

class WeatherService {
 public:
  void begin(LocalConfig*,Storage*);void loop();void configChanged();
  WeatherInfo info()const;
 private:
  LocalConfig*c_=nullptr;SemaphoreHandle_t mutex_=nullptr;Storage*store_=nullptr;WeatherInfo info_;uint32_t lastFetch_=0,generation_=0;bool workerRunning_=false,pendingLocationSave_=false;String postalCode_,language_,cachedPostalCode_,cachedPlace_,pendingPostal_;float cachedLat_=0,cachedLon_=0,pendingLat_=0,pendingLon_=0;String pendingPlace_;
  static void workerThunk(void*);
  void fetchNow();
  bool geocode(const String&,const String&,float&,float&,String&,String&);
  bool forecast(float,float,const String&,const String&,WeatherInfo&);
};
