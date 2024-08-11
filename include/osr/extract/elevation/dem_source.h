#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "osr/extract/elevation/elevation.h"
#include "osr/point.h"
#include "geo/webmercator.h"

namespace osr::elevation {

struct dem_source {
  dem_source();
  ~dem_source();
  dem_source(dem_source const&) = delete;
  dem_source& operator=(dem_source const&) = delete;
  dem_source(dem_source&&) = delete;
  dem_source& operator=(dem_source&&) = delete;

  void add_file(std::string const& filename);

  elevation_t get(geo::latlng const& loc) const;

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};

ElevationChange sample_elevation_change(point& from,point& to, elevation::dem_source& dem,
                                        double sampl_interval);

double mc_scale_factor(geo::merc_xy const& mc);

}  // namespace osr::elevation