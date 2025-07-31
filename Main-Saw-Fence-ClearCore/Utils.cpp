#include "Utils.h"
#include <Arduino.h>

float convertToInches(float value, UnitType unit) {
  if (unit == UNIT_MILLIMETERS) {
    return value * MM_TO_INCH_FACTOR;
  }
  return value;
}

float convertFromInches(float valueInInches, UnitType targetUnit) {
  if (targetUnit == UNIT_MILLIMETERS) {
    return valueInInches / MM_TO_INCH_FACTOR;
  }
  return valueInInches;
}

float convertUnits(float value, UnitType from, UnitType to) {
  if (from == to) return value;
  if (from == UnitType::UNIT_INCHES && to == UnitType::UNIT_MILLIMETERS)
    return value * 25.4;
  if (from == UnitType::UNIT_MILLIMETERS && to == UnitType::UNIT_INCHES)
    return value / 25.4;
  // Add other unit pairs if needed
  return value;
}

String getUnitString(UnitType unit) {
  switch (unit) {
    case UNIT_INCHES:
      return " in";
    case UNIT_MILLIMETERS:
      return " mm";
    default:
      return "";
  }
}

String getUnitWordStringFromUnit(UnitType unit){
  switch (unit) {
    case UNIT_INCHES:
      return "inches";
    case UNIT_MILLIMETERS:
      return "millimeters";
    default:
      return "";
  }
}

UnitType getUnitFromString(String str) {
  if (str == "inches") {
    return UNIT_INCHES;
  } else if (str == "millimeters") {
    return UNIT_MILLIMETERS;
  } else {
    return UNIT_UNKNOWN;
  }
}