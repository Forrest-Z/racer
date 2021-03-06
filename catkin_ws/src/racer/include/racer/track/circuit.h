#pragma once

#include <vector>

#include "racer/track/occupancy_grid.h"
#include "racer/space_exploration/space_exploration.h"
#include "racer/track/analysis.h"

namespace racer::track
{

class circuit
{
public:
  const size_t number_of_waypoints;
  const std::vector<racer::math::point> waypoints;
  const std::shared_ptr<occupancy_grid> grid;
  const double waypoint_radius;

private:
  const double waypoint_radius_sq_;
  std::vector<double> remaining_distances_;

public:
  circuit(const std::vector<racer::math::point> list_of_waypoints, const double waypoint_radius,
          const std::shared_ptr<occupancy_grid> grid)
    : number_of_waypoints(list_of_waypoints.size())
    , waypoints(list_of_waypoints)
    , grid{ grid }
    , waypoint_radius(waypoint_radius)
    , waypoint_radius_sq_(waypoint_radius * waypoint_radius)
  {
    double remaining_distance = 0;

    for (size_t n = waypoints.size() - 1; n > 0; --n)
    {
      remaining_distances_.push_back(remaining_distance);
      remaining_distance += (waypoints[n - 1]).distance(waypoints[n]);
    }

    remaining_distances_.push_back(remaining_distance);
    std::reverse(remaining_distances_.begin(), remaining_distances_.end());
  }

  circuit(const circuit &other) = delete;
  circuit &operator=(const circuit &other) = delete;

  circuit(circuit &&other) = delete;
  circuit &operator=(circuit &&other) = delete;

  bool passes_waypoint(const racer::math::point &position, std::size_t waypoint_index) const
  {
    // I assume that waypoint_index is always in the bounds
    return distance_to_waypoint_sq(position, waypoint_index) < waypoint_radius_sq_ &&
           grid->are_directly_visible(position, waypoints[waypoint_index]);
  }

  double distance_to_waypoint_sq(const math::vector &position, std::size_t n) const
  {
    if (n >= waypoints.size())
      return 0;

    return position.distance_sq(waypoints[n]);
  }

  double distance_to_waypoint(const math::vector &position, std::size_t n) const
  {
    return std::sqrt(distance_to_waypoint_sq(position, n));
  }

  double remaining_distance_estimate(std::size_t n) const
  {
    if (n >= remaining_distances_.size())
      return 0;

    return remaining_distances_[n];
  }

  std::unique_ptr<circuit> for_waypoint_subset(std::size_t from, std::size_t n)
  {
    std::vector<racer::math::point> waypoints_subset;
    for (std::size_t i = 0; i < n; ++i)
    {
      waypoints_subset.push_back(waypoints[(from + i) % waypoints.size()]);
    }

    return std::make_unique<circuit>(waypoints_subset, waypoint_radius, grid);
  }
};

}  // namespace racer::track
