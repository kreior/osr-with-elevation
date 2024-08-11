#pragma once

#include <cstdint>

namespace osr::elevation {

using elevation_t = int16_t;
constexpr elevation_t NO_ELEVATION_DATA = -32767;
constexpr double sampling_interval = 30;

static constexpr uint16_t SCALING_FACTOR = 4; // Define experimentally
static constexpr uint8_t MAX_BASE_VALUE = 15; // Maximum value for 4 bits
static constexpr elevation_t MAX_VALUE = MAX_BASE_VALUE * SCALING_FACTOR;

struct ElevationChange {
private:
  uint8_t data; // 8-bit integer to hold descent and elevation

  static constexpr uint8_t DESCENT_MASK = 0x0F;     // 00001111
  static constexpr uint8_t ELEVATION_MASK = 0xF0;   // 11110000
  static constexpr uint8_t ELEVATION_SHIFT = 4;     // Shift for elevation

public:
  constexpr ElevationChange() : data(0) {}
  constexpr explicit ElevationChange(uint8_t initialData) : data(initialData) {}

  constexpr void addElevation(uint16_t elev) {
    uint16_t current_elevation = getElevation();
    uint16_t new_elevation = current_elevation + elev;

    if (new_elevation > MAX_VALUE) {
      new_elevation = MAX_VALUE;
    }

    // Scale and set the new value
    uint8_t scaled_value = new_elevation / SCALING_FACTOR;
    data = (data & DESCENT_MASK) | (scaled_value << ELEVATION_SHIFT);
  }

  constexpr void addDescent(uint16_t desc) {
    uint16_t current_descent = getDescent();
    uint16_t new_descent = current_descent + desc;

    if (new_descent > MAX_VALUE) {
      new_descent = MAX_VALUE;
    }

    // Scale and set the new value
    uint8_t scaled_value = new_descent / SCALING_FACTOR;
    data = (data & ELEVATION_MASK) | (scaled_value & DESCENT_MASK);
  }

  constexpr uint16_t getElevation() const {
    return ((data & ELEVATION_MASK) >> ELEVATION_SHIFT) * SCALING_FACTOR;
  }

  constexpr uint16_t getDescent() const {
    return (data & DESCENT_MASK) * SCALING_FACTOR;
  }

  constexpr uint16_t getTotalElevationChange() const {
    return getElevation() + getDescent();
  }

  constexpr double getElevationFraction() const {
    return static_cast<double>(getElevation()) / getTotalElevationChange();
  }

  constexpr double getDescentFraction() const {
    return static_cast<double>(getDescent()) / getTotalElevationChange();
  }

  constexpr uint8_t getData() const {
    return data;
  }

  constexpr void reset() {
    data = 0;
  }

  constexpr void reverseDirection() {
    uint8_t elevation = (data & ELEVATION_MASK) >> ELEVATION_SHIFT;
    uint8_t descent = data & DESCENT_MASK;
    data = (descent << ELEVATION_SHIFT) | (elevation & DESCENT_MASK);
  }
};


constexpr double getAverageSlope(ElevationChange const& elev,
                                 uint16_t const dist){
  if(dist == 0){
    return 0.0;
  }
  return elev.getTotalElevationChange() / static_cast<double>(dist) * 100;
}

}  // namespace osr::elevation
