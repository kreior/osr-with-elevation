#include "osr/ways.h"

#include <boost/qvm/map_mat_vec.hpp>

#include "cista/io.h"
#include "utl/parallel_for.h"
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

namespace osr {

ways::ways(std::filesystem::path p, cista::mmap::protection const mode)
    : p_{std::move(p)},
      mode_{mode},
      r_{mode == cista::mmap::protection::READ
             ? routing::read(p_)
             : cista::wrapped<routing>{cista::raw::make_unique<routing>()}},
      node_to_osm_{mm("node_to_osm.bin")},
      way_osm_idx_{mm("way_osm_idx.bin")},
      way_polylines_{mm_vec<point>{mm("way_polylines_data.bin")},
                     mm_vec<std::uint64_t>{mm("way_polylines_index.bin")}},
      way_osm_nodes_{mm_vec<osm_node_idx_t>{mm("way_osm_nodes_data.bin")},
                     mm_vec<std::uint64_t>{mm("way_osm_nodes_index.bin")}},
      strings_{mm_vec<char>(mm("strings_data.bin")),
               mm_vec<std::uint64_t>(mm("strings_idx.bin"))},
      way_names_{mm("way_names.bin")} {}

void ways::add_restriction(std::vector<resolved_restriction>& rs) {
  using it_t = std::vector<resolved_restriction>::iterator;
  utl::sort(rs, [](auto&& a, auto&& b) { return a.via_ < b.via_; });
  utl::equal_ranges_linear(
      begin(rs), end(rs), [](auto&& a, auto&& b) { return a.via_ == b.via_; },
      [&](it_t const& lb, it_t const& ub) {
        auto const range = std::span{lb, ub};
        r_->node_restrictions_.resize(to_idx(range.front().via_) + 1U);
        r_->node_is_restricted_.set(range.front().via_, true);

        for (auto const& x : range) {
          if (x.type_ == resolved_restriction::type::kNo) {
            r_->node_restrictions_[x.via_].push_back(
                restriction{r_->get_way_pos(x.via_, x.from_),
                            r_->get_way_pos(x.via_, x.to_)});
          } else /* kOnly */ {
            for (auto const [i, from] :
                 utl::enumerate(r_->node_ways_[x.via_])) {
              for (auto const [j, to] :
                   utl::enumerate(r_->node_ways_[x.via_])) {
                if (x.from_ == from && x.to_ != to) {
                  r_->node_restrictions_[x.via_].push_back(restriction{
                      static_cast<way_pos_t>(i), static_cast<way_pos_t>(j)});
                }
              }
            }
          }
        }
      });
  r_->node_restrictions_.resize(node_to_osm_.size());
}

void ways::connect_ways(elevation::dem_source& dem) {
  auto pt = utl::get_active_progress_tracker_or_activate("osr");

  {  // Assign graph node ids to every node with >1 way.
    pt->status("Create graph nodes")
        .in_high(node_way_counter_.size())
        .out_bounds(50, 60);

    auto node_idx = node_idx_t{0U};
    node_way_counter_.multi_.for_each_set_bit([&](std::uint64_t const b_idx) {
      auto const i = osm_node_idx_t{b_idx};
      node_to_osm_.push_back(i);
      ++node_idx;
      pt->update(b_idx);
    });
    r_->node_is_restricted_.resize(to_idx(node_idx));
  }

  // Build edges.
  {
    pt->status("Connect ways")
        .in_high(way_osm_nodes_.size())
        .out_bounds(60, 90);
    auto node_ways = mm_paged_vecvec<node_idx_t, way_idx_t>{
        cista::paged<mm_vec32<way_idx_t>>{
            mm_vec32<way_idx_t>{mm("tmp_node_ways_data.bin")}},
        mm_vec<cista::page<std::uint32_t, std::uint16_t>>{
            mm("tmp_node_ways_index.bin")}};
    auto node_in_way_idx = mm_paged_vecvec<node_idx_t, std::uint16_t>{
        cista::paged<mm_vec32<std::uint16_t>>{
            mm_vec32<std::uint16_t>{mm("tmp_node_in_way_idx_data.bin")}},
        mm_vec<cista::page<std::uint32_t, std::uint16_t>>{
            mm("tmp_node_in_way_idx_index.bin")}};
    std::mutex mtx_connect_ways;
    node_ways.resize(node_to_osm_.size());
    node_in_way_idx.resize(node_to_osm_.size());

    r_->way_node_dist_.resize(way_osm_idx_.size());
    r_->way_node_elevation_.resize(way_osm_idx_.size());
    r_->way_nodes_.resize(way_osm_idx_.size());

    auto zipped = utl::zip(way_osm_idx_, way_osm_nodes_, way_polylines_);
    using ZippedElementType = decltype(*zipped.begin());
    std::vector<ZippedElementType> zipped_vector(zipped.begin(), zipped.end());
    
    std::vector<std::pair<size_t, ZippedElementType>> indexed_zipped_vector;
    indexed_zipped_vector.reserve(zipped_vector.size());
    for (size_t i = 0; i < zipped_vector.size(); ++i) {
      indexed_zipped_vector.emplace_back(i, zipped_vector[i]);
    }

    utl::parallel_for(indexed_zipped_vector, [this, &node_ways, &node_in_way_idx, &mtx_connect_ways, pt, &dem](auto const& idx_tup) {
    auto const& [idx, tup] = idx_tup;
    auto const& [osm_way_idx, osm_nodes, polyline] = tup;

      auto way_idx = way_idx_t{idx};
      auto pred_pos = std::make_optional<point>();
      auto from = node_idx_t::invalid();
      auto distance = 0.0;
      elevation::ElevationChange elevationChange;
      auto i = std::uint16_t{0U};
      
      std::vector<uint16_t> local_dists;
      std::vector<uint8_t> local_elevs;
      std::vector<std::tuple<node_idx_t, way_idx_t, u_int16_t>> local_way_ids;
      for (auto const [osm_node_idx, pos] : utl::zip(osm_nodes, polyline)) {
        if (pred_pos.has_value()) {
          distance += geo::distance(pos, *pred_pos);
          elevationChange += elevation::sample_elevation_change(*pred_pos, pos, dem, elevation::sampling_interval);
        }

        if (node_way_counter_.is_multi(to_idx(osm_node_idx))) {
          auto const to = get_node_idx(osm_node_idx);
          local_way_ids.push_back({to,way_idx, i});
          if (from != node_idx_t::invalid()) {
            local_dists.push_back(static_cast<std::uint16_t>(std::round(distance)));//here the distance is aggregated
            local_elevs.push_back(elevationChange.getData());
          }

          distance = 0.0;
          elevationChange.reset();
          from = to;

          if (i == std::numeric_limits<std::uint16_t>::max()) {
            fmt::println("error: way with {} nodes", osm_way_idx);
          }

          ++i;
        }

        pred_pos = pos;
      }

      //sync here
      std::unique_lock<std::mutex> lock(mtx_connect_ways);
      for (auto const& x : local_dists) {
        r_->way_node_dist_[way_idx].push_back(x);
      }
      for (auto const& x : local_elevs) {
        r_->way_node_elevation_[way_idx]. push_back(x);
      }
      for (auto const& [node_id, way_id, node_in_way_id] : local_way_ids) {
        node_ways[node_id].push_back(way_id);
        node_in_way_idx[node_id].push_back(node_in_way_id);
        r_->way_nodes_[way_idx].push_back(node_id);
      }
      lock.unlock();
      pt->increment();
    });

    auto way_ids_it = node_ways.begin();
    auto node_ids_it = node_in_way_idx.begin();

    //Need to resort based on local way ID because of multithreading
    for (; way_ids_it != node_ways.end(); ++way_ids_it, ++node_ids_it) {
      // Create and populate a vector of pairs directly
      std::vector<std::pair<way_idx_t, u_int16_t>> collect;
      collect.reserve((*way_ids_it).size());

      for (size_t i = 0; i < (*way_ids_it).size(); ++i) {
        collect.emplace_back((*way_ids_it)[i], (*node_ids_it)[i]);
      }

      // Sort based on way_id
      std::ranges::sort(collect, [](const auto& a, const auto& b) {
          return a.first < b.first;
      });

      std::vector<way_idx_t> way_ids_sorted(collect.size());
      std::vector<u_int16_t> node_ids_sorted(collect.size());
      for (size_t i = 0; i < collect.size(); ++i) {
        way_ids_sorted[i] = collect[i].first;
        node_ids_sorted[i] = collect[i].second;
      }

      r_->node_ways_.emplace_back(std::move(way_ids_sorted));
      r_->node_in_way_idx_.emplace_back(std::move(node_ids_sorted));
    }
  }

  auto e = std::error_code{};
  std::filesystem::remove(p_ / "tmp_node_ways_data.bin", e);
  std::filesystem::remove(p_ / "tmp_node_ways_index.bin", e);
  std::filesystem::remove(p_ / "tmp_node_in_way_idx_data.bin", e);
  std::filesystem::remove(p_ / "tmp_node_in_way_idx_index.bin", e);
}

void ways::sync() {
  node_to_osm_.mmap_.sync();
  way_osm_idx_.mmap_.sync();
  way_polylines_.data_.mmap_.sync();
  way_polylines_.bucket_starts_.mmap_.sync();
  way_osm_nodes_.data_.mmap_.sync();
  way_osm_nodes_.bucket_starts_.mmap_.sync();
  strings_.data_.mmap_.sync();
  strings_.bucket_starts_.mmap_.sync();
  way_names_.mmap_.sync();
}

cista::wrapped<ways::routing> ways::routing::read(
    std::filesystem::path const& p) {
  return cista::read<ways::routing>(p / "routing.bin");
}

void ways::routing::write(std::filesystem::path const& p) const {
  return cista::write(p / "routing.bin", *this);
}

}  // namespace osr