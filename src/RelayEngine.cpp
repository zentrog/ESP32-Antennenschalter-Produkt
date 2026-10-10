#include "RelayEngine.h"
#include <driver/gpio.h>
#include "BoardProfiles.h"

RelayEngine* RelayEngine::instance_=nullptr;
static String RL(LocalConfig* c,const char* de,const char* en){return c&&c->language=="en"?String(en):String(de);}

void RelayEngine::begin(LocalConfig*c,Storage*s){
  cfg_=c;store_=s;instance_=this;
  esp_timer_create_args_t args{};args.callback=&RelayEngine::motorTimerThunk;args.arg=this;args.dispatch_method=ESP_TIMER_TASK;args.name="motorCutoff";
  esp_timer_create(&args,&motorTimer_);
}
void RelayEngine::motorTimerThunk(void* arg){
  auto*self=static_cast<RelayEngine*>(arg);
  if(self->motorGpio_>=0) gpio_set_level((gpio_num_t)self->motorGpio_,self->motorOffLevel_);
  self->motorTimerExpired_=true;
}
bool RelayEngine::txActive() const {
  if(!cfg_ || !cfg_->txInterlock.enabled) return false;
  int v=digitalRead(cfg_->txInterlock.gpio);
  return cfg_->txInterlock.activeHigh ? v==HIGH : v==LOW;
}
uint32_t RelayEngine::motorRemainingMs() const {
  if(!state_.motorRunning) return 0;
  int32_t left=(int32_t)(motorEnd_-millis());
  return left>0?(uint32_t)left:0U;
}
String RelayEngine::selectedInGroup(const String& group) const {
  for(const auto&a:state_.activeSelections)if(a.group==group)return a.functionId;
  return "";
}
bool RelayEngine::isFunctionActive(const String&id) const {
  for(const auto&a:state_.activeSelections)if(a.functionId==id)return true;
  return false;
}
RelayConfig* RelayEngine::relay(const String&id){for(auto&r:cfg_->relays)if(r.enabled&&r.id==id)return &r;return nullptr;}
FunctionConfig* RelayEngine::function(const String&id){for(auto&f:cfg_->functions)if(f.enabled&&f.id==id)return &f;return nullptr;}
void RelayEngine::setRelay(RelayConfig&r,bool on){digitalWrite(r.gpio,(r.activeLow?!on:on)?HIGH:LOW);}
void RelayEngine::applyStormOutputs(){for(const auto&d:cfg_->devices){if(!d.enabled||d.stormRelayId.isEmpty())continue;RelayConfig*r=relay(d.stormRelayId);if(r)setRelay(*r,d.stormRelayOn);}}
void RelayEngine::safeInit(){
  for(auto&r:cfg_->relays){if(!r.enabled)continue;digitalWrite(r.gpio,r.activeLow?HIGH:LOW);pinMode(r.gpio,OUTPUT);setRelay(r,false);}
  if(cfg_->txInterlock.enabled) pinMode(cfg_->txInterlock.gpio,INPUT);
  state_.motorRunning=false;state_.motorFunctionId="";runningRelay_="";pendingToken_="";motorGpio_=-1;motorTimerExpired_=false;
}
void RelayEngine::emergencyOff(){
  if(motorTimer_) esp_timer_stop(motorTimer_);
  restorePending_=false;
  bool wasMotor=state_.motorRunning;
  for(auto&r:cfg_->relays)if(r.enabled)setRelay(r,false);
  state_.motorRunning=false;state_.motorFunctionId="";runningRelay_="";pendingToken_="";motorGpio_=-1;motorTimerExpired_=false;
  store_->clearSelections();state_.activeSelections.clear();state_.activeAntennaId="";
  if(wasMotor){store_->motorUnknown();state_.polarization="UNKNOWN";}
  if(state_.stormMode)applyStormOutputs();
}

bool RelayEngine::testRelay(const String& relayId,String&err){
  if(state_.stormMode){err=RL(cfg_,"Relais-Test im Gewittermodus gesperrt","Relay test is blocked in storm mode");return false;}
  if(state_.motorRunning){err=RL(cfg_,"Relais-Test während Motorlauf gesperrt","Relay test is blocked while the motor is running");return false;}
  if(txActive()){err=RL(cfg_,"Relais-Test während TX gesperrt","Relay test is blocked during TX");return false;}
  RelayConfig* target=relay(relayId);if(!target){err=RL(cfg_,"Relais nicht vorhanden/aktiv","Relay does not exist or is not active");return false;}
  auto selections=state_.activeSelections;
  for(auto&r:cfg_->relays)if(r.enabled)setRelay(r,false);
  delay(100);setRelay(*target,true);delay(400);setRelay(*target,false);delay(100);
  state_.activeSelections.clear();state_.activeAntennaId="";
  for(const auto&a:selections){String e;execute(a.functionId,e);if(e.length()){err=RL(cfg_,"Test beendet, Wiederherstellung fehlgeschlagen: ","Test finished, restore failed: ")+e;return false;}}
  return true;
}
bool RelayEngine::testGpio(int gpio,String&err){
  if(state_.stormMode){err=RL(cfg_,"GPIO-Test im Gewittermodus gesperrt","GPIO test is blocked in storm mode");return false;}
  if(state_.motorRunning){err=RL(cfg_,"GPIO-Test während Motorlauf gesperrt","GPIO test is blocked while a motor is running");return false;}
  if(txActive()){err=RL(cfg_,"GPIO-Test während TX gesperrt","GPIO test is blocked during TX");return false;}
  if(!state_.activeSelections.empty()){err=RL(cfg_,"GPIO-Test nur bei ausgeschalteten Antennenfunktionen möglich","GPIO test is only available while antenna functions are off");return false;}
  if(cfg_->txInterlock.enabled&&cfg_->txInterlock.gpio==gpio){err=RL(cfg_,"GPIO ist als TX-Eingang belegt","GPIO is assigned as the TX input");return false;}
  for(const auto&r:cfg_->relays)if(r.enabled&&r.gpio==gpio){err=RL(cfg_,"GPIO ist bereits einem Relais zugeordnet; bitte dessen Relais-Test verwenden","GPIO is already assigned to a relay; use that relay's test instead");return false;}
  String reason;if(!BoardProfiles::diagnosticOutputAllowed(*cfg_,gpio,reason)){err=RL(cfg_,"GPIO ist für den Diagnosetest nicht nutzbar: ","GPIO cannot be used for diagnostic testing: ")+reason;return false;}
  digitalWrite(gpio,HIGH);pinMode(gpio,OUTPUT);digitalWrite(gpio,LOW);delay(400);digitalWrite(gpio,HIGH);
  // Keep ordinary relay candidates actively HIGH (inactive for low-trigger
  // relay inputs). GPIO2 is also held HIGH after startup: Espressif documents
  // that it is ignored in normal SPI boot when GPIO0 is HIGH. Keep the other
  // boot/UART-sensitive pins as inputs after testing.
  if(gpio==0||gpio==1||gpio==3||gpio==5||gpio==12||gpio==15)pinMode(gpio,INPUT);
  return true;
}
void RelayEngine::restore(bool restoreSelections){
  RuntimeState p=store_->loadState();
  restorePending_=false;
  state_.stormMode=p.stormMode;
  if(p.motorRunning){store_->motorUnknown();state_.polarization="UNKNOWN";store_->addError("MOTOR_INTERRUPTED",RL(cfg_,"Stromausfall/Reset während H/V-Bewegung; Position unbekannt","Power loss/reset during H/V movement; position unknown"));}
  else state_.polarization=p.polarization;
  if(state_.stormMode){ emergencyOff(); state_.stormMode=true; applyStormOutputs(); return; }
  if(!restoreSelections){state_.activeSelections.clear();state_.activeAntennaId="";return;}
  auto saved=p.activeSelections;
  state_.activeSelections.clear();state_.activeAntennaId="";
  // Preserve the durable desired state if TX interlock temporarily prevents
  // restoring outputs during boot. Retry once the interlock is released.
  if(!saved.empty()&&txActive()){restorePending_=true;return;}
  for(const auto&a:saved){String e;execute(a.functionId,e);if(e.length())store_->addError("RESTORE_FAILED",a.group+": "+e);}
}
bool RelayEngine::execute(const String&id,String&err){
  FunctionConfig*f=function(id);
  if(!f||!f->visible){err=RL(cfg_,"Funktion nicht verfügbar","Function not available");return false;}

  if(f->type==FunctionType::Storm){err=RL(cfg_,"Gewittermodus wird systemweit vom aktiven Master geschaltet","Storm mode is controlled system-wide by the active master");return false;}

  if(state_.stormMode){err=RL(cfg_,"Gewittermodus ist aktiv","Storm mode is active");return false;}
  RelayConfig*r=relay(f->relayId);
  if(!r){err=RL(cfg_,"Relais nicht verfügbar","Relay not available");return false;}
  if(txActive()){err=RL(cfg_,"Schalten während TX durch Interlock gesperrt","Switching is blocked by the TX interlock");return false;}

  if(f->type==FunctionType::Antenna){
    if(state_.motorRunning){err=RL(cfg_,"Antennenwechsel während Motorlauf gesperrt","Antenna switching is blocked while the motor is running");return false;}
    for(auto&x:cfg_->functions)if(x.enabled&&x.type==FunctionType::Antenna&&x.group==f->group){auto*rr=relay(x.relayId);if(rr)setRelay(*rr,false);}
    delay(100);setRelay(*r,true);
    bool found=false;for(auto&a:state_.activeSelections)if(a.group==f->group){a.functionId=f->id;found=true;break;}
    if(!found)state_.activeSelections.push_back({f->group,f->id});
    state_.activeAntennaId=state_.activeSelections.empty()?"":state_.activeSelections[0].functionId;
    store_->saveSelection(f->group,f->id);return true;
  }

  if(state_.motorRunning){err=RL(cfg_,"Es läuft bereits eine Zeitaktion","A timed action is already running");return false;}
  if(!f->requiresFunctionId.isEmpty()&&!isFunctionActive(f->requiresFunctionId)){err=RL(cfg_,"Funktion ist bei der aktuellen Antenne gesperrt","Function is locked for the current antenna");return false;}
  for(auto&x:cfg_->functions)if(x.enabled&&x.type==FunctionType::Timed){auto*rr=relay(x.relayId);if(rr)setRelay(*rr,false);}
  store_->motorStart();setRelay(*r,true);state_.motorRunning=true;state_.motorFunctionId=f->id;runningRelay_=f->relayId;pendingToken_=f->stateToken.length()?f->stateToken:f->label;
  uint32_t dur=f->durationMs>100000U?100000U:f->durationMs;
  motorEnd_=millis()+dur;motorGpio_=r->gpio;motorOffLevel_=r->activeLow?HIGH:LOW;motorTimerExpired_=false;
  if(motorTimer_){esp_timer_stop(motorTimer_);esp_timer_start_once(motorTimer_,(uint64_t)dur*1000ULL);}
  return true;
}

void RelayEngine::setStormMode(bool active){
  if(active){
    if(state_.stormMode){applyStormOutputs();return;}
    store_->saveStormSnapshot(state_);
    emergencyOff();
    state_.stormMode=true;store_->saveStorm(true);applyStormOutputs();
    return;
  }
  if(!state_.stormMode){store_->saveStorm(false);return;}
  // Schutzrelais zuerst lösen, dann den Zustand vor dem Gewitter wieder herstellen.
  for(auto&r:cfg_->relays)if(r.enabled)setRelay(r,false);
  state_.stormMode=false;store_->saveStorm(false);state_.activeSelections.clear();state_.activeAntennaId="";
  RuntimeState saved;bool have=store_->loadStormSnapshot(saved);bool ok=true;
  if(have){
    state_.polarization=saved.polarization;
    if(!saved.polarization.isEmpty())store_->motorFinished(saved.polarization);
    for(const auto&a:saved.activeSelections){String e;if(!execute(a.functionId,e)){ok=false;store_->addError("STORM_RESTORE_FAILED",a.group+": "+e);}}
  }
  if(ok)store_->clearStormSnapshot();
}
bool RelayEngine::clearGroup(const String&group,String&err){
  if(group.isEmpty()){err=RL(cfg_,"Schaltgruppe fehlt","Switching group is missing");return false;}
  if(state_.motorRunning){err=RL(cfg_,"Schaltgruppe kann während einer Zeitaktion nicht gelöst werden","Switching group cannot be released while a timed action is running");return false;}
  if(txActive()){err=RL(cfg_,"Schalten während TX durch Interlock gesperrt","Switching is blocked by the TX interlock");return false;}
  for(auto&x:cfg_->functions)if(x.enabled&&x.type==FunctionType::Antenna&&x.group==group){auto*r=relay(x.relayId);if(r)setRelay(*r,false);}
  std::vector<ActiveSelection> kept;for(const auto&a:state_.activeSelections)if(a.group!=group)kept.push_back(a);state_.activeSelections=kept;state_.activeAntennaId=state_.activeSelections.empty()?"":state_.activeSelections[0].functionId;store_->saveSelection(group,"");return true;
}
void RelayEngine::loop(){
  if(restorePending_&&!state_.stormMode&&!state_.motorRunning&&!txActive())restore(true);
  if(!state_.motorRunning)return;
  if(!motorTimerExpired_ && (int32_t)(millis()-motorEnd_)<0)return;
  auto*r=relay(runningRelay_);if(r)setRelay(*r,false);if(motorTimer_)esp_timer_stop(motorTimer_);motorTimerExpired_=false;motorGpio_=-1;state_.motorRunning=false;state_.polarization=pendingToken_;store_->motorFinished(state_.polarization);state_.motorFunctionId="";runningRelay_="";pendingToken_="";
}
