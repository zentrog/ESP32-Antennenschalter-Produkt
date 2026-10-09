#pragma once
#include "Model.h"
#include "Storage.h"
#include <esp_timer.h>

class RelayEngine {
 public:
  void begin(LocalConfig* cfg,Storage* store);
  void safeInit();
  void restore(bool restoreSelections=true);
  void loop();
  bool execute(const String& id,String& err);
  bool clearGroup(const String& group,String& err);
  void setStormMode(bool active);
  void emergencyOff();
  bool testRelay(const String& relayId,String& err);
  bool testGpio(int gpio,String& err);
  bool txActive() const;
  bool stormActive() const { return state_.stormMode; }
  uint32_t motorRemainingMs() const;
  bool isFunctionActive(const String& id) const;
  String selectedInGroup(const String& group) const;
  const RuntimeState& state() const {return state_;}
 private:
  LocalConfig* cfg_=nullptr; Storage* store_=nullptr; RuntimeState state_;
  uint32_t motorEnd_=0; String runningRelay_, pendingToken_;
  esp_timer_handle_t motorTimer_=nullptr;
  volatile bool motorTimerExpired_=false;
  int motorGpio_=-1; int motorOffLevel_=LOW;
  static RelayEngine* instance_;
  static void motorTimerThunk(void* arg);

  RelayConfig* relay(const String& id); FunctionConfig* function(const String& id);
  void setRelay(RelayConfig& r,bool on);
  void applyStormOutputs();
};
