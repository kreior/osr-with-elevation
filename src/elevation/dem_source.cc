#include <vector>

#include "boost/algorithm/string/case_conv.hpp"
#include "boost/filesystem.hpp"

#include "osr/extract/elevation/dem_grid.h"
#include "osr/extract/elevation/dem_source.h"

namespace fs = boost::filesystem;

namespace osr::elevation {

struct dem_source::impl {
  impl() = default;

  void add_file(std::string const& filename) {
    auto const path = fs::path{filename};
    if (fs::is_directory(path)) {
      for (auto const& de : fs::directory_iterator(path)) {
        if (boost::to_lower_copy(de.path().extension().string()) == ".hdr") {
          add_grid_file(de.path());
        }
      }
    } else {
      add_grid_file(path);
    }
  }

  void add_grid_file(fs::path const& path) {
    grids_.emplace_back(path.string());
  }

  elevation_t get(geo::latlng const& loc) {
    for (auto const& grid : grids_) {
      auto const data = grid.get(loc);
      if (data != NO_ELEVATION_DATA) {
        return data;
      }
    }
    return NO_ELEVATION_DATA;
  }

  std::vector<dem_grid> grids_;
};

dem_source::dem_source() : impl_(std::make_unique<impl>()) {}

dem_source::~dem_source() = default;

void dem_source::add_file(std::string const& filename) {
  impl_->add_file(filename);
}

elevation_t dem_source::get(geo::latlng const& loc) const {
  return impl_->get(loc);
}

double mc_scale_factor(geo::merc_xy const& mc) {
  auto const lat_rad =
      2.0 * std::atan(std::exp(mc.y() / geo::kMercEarthRadius)) - (geo::kPI / 2);
  return std::cos(lat_rad);
}

ElevationChange sample_elevation_change(point& from,point& to,
                                        elevation::dem_source& dem,
                                        double sampl_interval) {

  auto start_pt = geo::latlng_to_merc(from.as_latlng());
  auto end_pt = geo::latlng_to_merc(to.as_latlng());
  auto const scaled_interval = sampl_interval * mc_scale_factor(start_pt);
  auto step = end_pt - start_pt;
  step.normalize();
  step *= scaled_interval;
  auto distance = geo::distance(from, to);

  elevation_t last_value = dem.get(from.as_latlng());
  ElevationChange elevChange;
  auto const update_elevation = [&](elevation_t value) {
    if (value >= last_value) {
      elevChange.addElevation(value - last_value);
    } else {
      elevChange.addDescent(last_value - value);
    }
    last_value = value;
  };
  auto pt = start_pt;
  auto const samples =
      static_cast<int>(std::floor(distance / sampl_interval));
  for (auto i = 1; i < samples; i++) {
    pt += step;
    auto const value = dem.get(geo::merc_to_latlng(pt));
    if (value != NO_ELEVATION_DATA) {
      update_elevation(value);
    }
  }
  update_elevation(dem.get(to.as_latlng()));

  return elevChange;
}

}  // namespace osr::elevation
