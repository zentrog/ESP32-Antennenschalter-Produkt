#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "Model.h"
#include "Storage.h"
#include "RelayEngine.h"
#include "Services.h"
#include "WifiManager.h"

class WebUi {
 public:
  void begin(LocalConfig*,SharedConfig*,Storage*,RelayEngine*,TimeService*,FederationService*,NewsService*,WeatherService*,WifiManager*,bool recovery);
  void loop();
 private:
  WebServer server_{80};LocalConfig*c_=nullptr;SharedConfig*s_=nullptr;Storage*store_=nullptr;RelayEngine*rel_=nullptr;TimeService*time_=nullptr;FederationService*fed_=nullptr;NewsService*news_=nullptr;WeatherService*weather_=nullptr;WifiManager*wifiMgr_=nullptr;bool recovery_=false;
  bool auth(bool configOnly=true);
  void routes();void setupRoutes();void sendJson(JsonDocument&,int=200);
  void apiSnapshot();void setupScan();void setupSaved();void setupAdd();void setupDelete();void setupMove();void apiEnterSetupMode();void apiStop();void apiFederationStop();void apiTestRelay();void apiFactoryReset();void apiControllerTestRelay();void apiFederationTestRelay();void apiTestGpio();void apiControllerTestGpio();void apiFederationTestGpio();void apiControllerRestart();void apiFederationRestart();void apiControllerFactoryReset();void apiFederationFactoryReset();void apiRestart();void apiControllerConfigGet();void apiControllerConfigPut();void apiFederationConfigGet();void apiFederationConfigPut();void apiConfigGet();void apiConfigPut();void apiSharedGet();void apiSharedPut();void apiLayoutItemPut();void apiBoard();void apiRouteActivate();void apiRouteDeactivate();void apiSystemStorm();void apiFederationStorm();void apiFederationGroupOff();void apiExecute();void apiPeers();void apiCombined();void apiSystemAdmission();void apiProvisionAuthorize();void apiSystemAdopt();void apiSystemMakeMaster();void apiFederationAdopt();void apiFederationMakeMaster();void apiErrors();void apiNews();void apiWeather();void apiWifi();void apiPullShared();void otaPage();void otaRollback();void otaInstallLatest();
};
