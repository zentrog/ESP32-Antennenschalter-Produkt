#pragma once
#include "Model.h"
#include <Preferences.h>

struct ErrorEntry { uint32_t epoch; String code; String text; };
struct WifiCredential { String ssid; String password; };

class Storage {
 public:
  bool begin();
  bool loadLocal(LocalConfig& c, String& err);
  bool saveLocal(LocalConfig& c, String& err);
  bool loadShared(SharedConfig& c, String& err);
  bool saveShared(SharedConfig& c, String& err);
  void defaults(LocalConfig& c, SharedConfig& s);
  bool validate(const LocalConfig& c, String& err) const;

  RuntimeState loadState();
  void saveSelection(const String& group,const String& id);
  void clearSelections();
  void motorStart();
  void motorFinished(const String& token);
  void motorUnknown();
  void saveStorm(bool active);
  void saveStormSnapshot(const RuntimeState& state);
  bool loadStormSnapshot(RuntimeState& state);
  void clearStormSnapshot();
  bool loadWeatherLocation(const String& postal,float& lat,float& lon,String& place);
  void saveWeatherLocation(const String& postal,float lat,float lon,const String& place);
  uint16_t allocateFailoverPriority();
  void observeFailoverPriority(uint16_t priority);
  void resetFailoverPrioritySequence();

  void addError(const String& code, const String& text, uint32_t epoch=0);
  std::vector<ErrorEntry> errors();
  void clearErrors();

  std::vector<WifiCredential> wifiNetworks();
  void ensureDefaultWifi();
  bool addOrUpdateWifi(const String& ssid,const String& pass,bool highestPriority,String& err);
  bool deleteWifi(size_t index,String& err);
  bool moveWifi(size_t index,int direction,String& err);
  bool replaceWifiNetworks(const std::vector<WifiCredential>& nets,String& err);
  void setForceSetup();
  bool consumeForceSetup();
  void factoryReset();

 private:
  Preferences state_, errors_, wifi_;
  bool readLocalFile(const char* path, LocalConfig& c, String& err);
  bool readSharedFile(const char* path, SharedConfig& c, String& err);
  bool writeLocalFile(const char* path,const LocalConfig& c,String& err);
  bool writeSharedFile(const char* path,const SharedConfig& c,String& err);
};
