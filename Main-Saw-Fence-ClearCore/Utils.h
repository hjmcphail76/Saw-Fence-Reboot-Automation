#pragma once

#include <Arduino.h>

const float MM_TO_INCH_FACTOR = 1.0 / 25.4;

enum UnitType {
  UNIT_INCHES,
  UNIT_MILLIMETERS,
  UNIT_UNKNOWN
};

struct Value {
  float val;
  UnitType unit;

  Value(float num, UnitType u)
    : val(num), unit(u) {}
};


// void initSDCard();
float convertToInches(float value, UnitType unit);
float convertFromInches(float valueInInches, UnitType targetUnit);
float convertUnits(float value, UnitType from, UnitType to);
String getUnitString(UnitType unit);
UnitType getUnitFromString(String str);