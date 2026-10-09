#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <vector>
#include "Storage.h"

struct ProvisioningCandidate {
  String controllerId;
  String boardProfile;
  String macAddress;
  String authorizedMode; // "", "join", "own"
  uint32_t lastSeen = 0;
  uint32_t nonce = 0;
  uint8_t mac[6] = {0};
  bool online = false;
  bool recovery = false; // existing follower of this system recovering WLAN
};

class WifiManager {
 public:
  void begin(Storage* store, LocalConfig* config);
  void loop();
  bool connected() const { return WiFi.status() == WL_CONNECTED && !setupMode_; }
  bool setupMode() const { return setupMode_; }
  bool provisioningWait() const { return provisioningWait_; }
  String setupSsid() const { return setupSsid_; }
  String currentSsid() const { return WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(); }
  IPAddress currentIp() const { return setupMode_ ? WiFi.softAPIP() : WiFi.localIP(); }

  void requestSetupMode();
  bool testAndAddNetwork(const String& ssid, const String& password, String& error, bool english=false);
  int scanNetworks();
  void processDns();

  std::vector<ProvisioningCandidate> provisioningCandidates() const;
  bool authorizeProvisioning(const String& controllerId, const String& mode, String& error);

  // Bridge for the ESP-NOW callback. Not part of the normal user-facing API.
  void onEspNowPacket(const uint8_t* mac, const uint8_t* data, int len);

 private:
  struct PendingProbe {
    uint8_t mac[6] = {0};
    char controllerId[20] = {0};
    char boardProfile[32] = {0};
    char systemId[33] = {0};
    char masterId[20] = {0};
    char role[12] = {0};
    uint32_t nonce = 0;
    uint8_t channel = 0;
  };
  struct PendingRequest {
    uint8_t mac[6] = {0};
    char controllerId[20] = {0};
    uint32_t nonce = 0;
    uint8_t assignment = 0;
  };
  struct OfferRecord {
    uint8_t mac[6] = {0};
    uint32_t nonce = 0;
    uint8_t channel = 0;
    uint8_t admission = 0;
    uint8_t assignment = 0;
  };

  Storage* store_ = nullptr;
  LocalConfig* config_ = nullptr;
  DNSServer dns_;
  bool setupMode_ = false;
  bool provisioningWait_ = false;
  bool espNowReady_ = false;
  String setupSsid_;
  uint32_t bootPressStart_ = 0;
  bool bootPressLatched_ = false;
  uint32_t lastCandidateSweep_ = 0;
  bool fallbackRecoveryMode_ = false;
  bool fallbackConnecting_ = false;
  uint8_t fallbackNetworkIndex_ = 0;
  uint32_t fallbackNextTryAt_ = 0;
  uint32_t fallbackConnectStartedAt_ = 0;
  uint32_t fallbackConnectedAt_ = 0;

  std::vector<ProvisioningCandidate> candidates_;
  volatile bool pendingProbeReady_ = false;
  volatile bool pendingRequestReady_ = false;
  PendingProbe pendingProbe_;
  PendingRequest pendingRequest_;

  volatile size_t offerCount_ = 0;
  OfferRecord offers_[8];
  volatile bool credentialReady_ = false;
  uint8_t credentialBuffer_[240] = {0};
  volatile int credentialLength_ = 0;
  uint32_t activeProvisionNonce_ = 0;

  bool connectKnownNetworks();
  bool connectOne(const WifiCredential& cred, uint32_t timeoutMs);
  void startSetupPortal(bool retrySavedNetworks=false);
  void serviceFallbackReconnect();
  bool networkVisible(const String& ssid, int scanCount) const;

  bool beginEspNow();
  void endEspNow();
  void processEspNow();
  void processPendingProbe();
  void processPendingRequest();
  void pruneProvisioningCandidates();
  bool tryEspNowProvisioning();
  bool sendProvisionProbe(uint8_t channel, uint32_t nonce);
  bool requestProvisioning(const OfferRecord& offer, uint8_t assignment);
  bool applyReceivedCredentials(String& error);
  bool sendOffer(const uint8_t* mac, uint32_t nonce, uint8_t assignment);
  bool sendCredentials(const ProvisioningCandidate& candidate, uint8_t assignment, uint32_t nonce, String& error);
  bool ensureEspNowPeer(const uint8_t* mac);
  ProvisioningCandidate* findCandidate(const String& controllerId, const uint8_t* mac=nullptr);
  String currentWifiPassword() const;
};
