#pragma once

#include <cinttypes>
#include <string_view>
#include "osr/types.h"

namespace osr {

enum class search_profile : std::uint8_t {
  kFoot,
  kWheelchair,
  kBike,
  kCar,
  kCarParking,
  kCarParkingWheelchair,
  kBikeSharing,
};

search_profile to_profile(std::string_view);

std::string_view to_str(search_profile);

elevation_profile to_elevation_profile(std::string_view);

std::string_view elevation_profile_to_str(elevation_profile);

}  // namespace osr
