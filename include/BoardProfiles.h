#pragma once
#include "Model.h"

struct BoardPin {
  String label;
  int gpio;
  PinClass pinClass;
  String noteDe;
  String noteEn;
  String side;
  int order;
};

struct BoardDefinition {
  String id;
  String nameDe;
  String nameEn;
  String noteDe;
  String noteEn;
  std::vector<BoardPin> pins;
};

class BoardProfiles {
 public:
  static const std::vector<BoardDefinition>& all();
  static const BoardDefinition* find(const String& id);
  static bool relayAllowed(const String& profileId, int gpio, String& reason, const String& language="de");
  static bool diagnosticOutputAllowed(const LocalConfig& config, int gpio, String& reason);
  static bool inputAllowed(const String& profileId, int gpio, String& reason, const String& language="de");
  static bool profileExists(const LocalConfig& config);
  static bool relayAllowed(const LocalConfig& config, int gpio, String& reason);
  static bool inputAllowed(const LocalConfig& config, int gpio, String& reason);
};
