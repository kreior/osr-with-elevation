#pragma once

#include "osr/types.h"

namespace osr {

  /**
 * Taken from https://osmand.net/docs/technical/osmand-file-formats/osmand-routing-xml#penalties-of-elevation-data
 * @param slope The slope in %
 * @param elev_profile The targeted elevation profile
 * @return a penalty >0 if the slope is traversable, 0 if impossible
 */
static constexpr double get_slope_penalty_foot(const double slope, const elevation_profile elev_profile){
    if (slope >= 0) {
      if (slope < 1.0) {
        switch (elev_profile) {
          case elevation_profile::hilly:
            return 61;
          default:
            return 1;
        }
      } else if (slope < 3.0) {
        switch (elev_profile) {
          case elevation_profile::flat:
            return 2;
          case elevation_profile::hilly:
            return 20;
          default:
            return 1;
        }
      } else if (slope < 7.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 4;
          case elevation_profile::flat:
            return 12;
          case elevation_profile::hilly:
            return 7;
          default:
            return 1;
        }
      } else if (slope < 13.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 8;
          case elevation_profile::flat:
            return 30;
          case elevation_profile::hilly:
            return 3;
          default:
            return 1;
        }
      } else if (slope < 25.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 10;
          case elevation_profile::flat:
            return 60;
          case elevation_profile::hilly:
            return 0.5;
          default:
            return 1;
        }
      } else {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 15;
          case elevation_profile::flat:
            return 74;
          case elevation_profile::hilly:
            return 0.3;
          default:
            return 1;
        }
      }
    } else {
      if (slope > -9.0) {
        return 5;
      } else if (slope > -17.0) {
        return 10;
      } else if (slope > -35.0) {
        return 17;
      } else if (slope > -60.0) {
        return 25;
      } else {
        return 40;
      }
    }
  }

/**
 * Taken from https://osmand.net/docs/technical/osmand-file-formats/osmand-routing-xml#penalties-of-elevation-data
 * @param slope The slope in %
 * @param elev_profile The targeted elevation profile
 * @return a penalty >0 if the slope is traversable, 0 if impossible
 */
static constexpr double get_slope_penalty_bike(double slope, elevation_profile elev_profile){
    if (slope >= 0) {
      if (slope < 1.0) {
        switch (elev_profile) {
          case elevation_profile::hilly:
            return 61;
          default:
            return 1;
        }
      } else if (slope < 3.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
          case elevation_profile::flat:
            return 2;
          case elevation_profile::hilly:
            return 19.7;
          default:
            return 1;
        }
      } else if (slope < 7.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 8;
          case elevation_profile::flat:
            return 12;
          case elevation_profile::hilly:
            return 7.5;
          default:
            return 1;
        }
      } else if (slope < 13.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 16;
          case elevation_profile::flat:
            return 30;
          case elevation_profile::hilly:
            return 3;
          default:
            return 1;
        }
      } else if (slope < 25.0) {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 32;
          case elevation_profile::flat:
            return 50;
          case elevation_profile::hilly:
            return 0.5;
          default:
            return 1;
        }
      } else {
        switch (elev_profile) {
          case elevation_profile::lessHilly:
            return 48;
          case elevation_profile::flat:
            return 74;
          case elevation_profile::hilly:
            return 0.3;
          default:
            return 1;
        }
      }
    } else {
      if (slope > -17.0) {
        return 6.4;
      } else if (slope > -60.0) {
        return 25;
      } else {
        return 0;
      }
    }
  }

}  // namespace osr
