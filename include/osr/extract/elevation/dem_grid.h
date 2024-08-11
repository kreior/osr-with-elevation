#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "osr/extract/elevation/elevation.h"
#include "osr/point.h"

namespace osr::elevation {

enum class pixel_type : uint8_t { int16, float32 };

union pixel_value {
  int16_t int16_;
  float float32_;
};

struct dem_grid {
  explicit dem_grid(std::string const& filename);
  ~dem_grid();
  dem_grid(dem_grid&& grid) noexcept;
  dem_grid(dem_grid const&) = delete;
  dem_grid& operator=(dem_grid const&) = delete;
  dem_grid& operator=(dem_grid&& grid) = delete;

  elevation_t get(geo::latlng const& loc) const;

  pixel_value get_raw(geo::latlng const& loc) const;
  pixel_type get_pixel_type() const;

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};

}  // namespace osr::elevation
