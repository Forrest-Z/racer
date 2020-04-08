#pragma once

#include <iostream>

#include "racer/math.h"
#include "racer/track/racing_line.h"
#include "racer/trajectory.h"
#include "racer/vehicle_configuration.h"
#include "racer/vehicle_model/motor_model.h"

namespace racer::following_strategies
{
template <typename State>
class pure_pursuit
{
public:
  pure_pursuit(double wheelbase) : wheelbase_{ wheelbase }
  {
  }

  racer::math::angle select_steering_angle(const State state, const racer::vehicle_configuration target) const
  {
    auto rear_axle_center = calculate_rear_axle_center(state.configuration());

    auto direction = target.location() - rear_axle_center;
    auto dist = target.location().distance(rear_axle_center);
    auto alpha = direction.angle() - state.configuration().heading_angle();

    return std::atan2(2 * wheelbase_ * sin(alpha), dist);
  }

private:
  const double wheelbase_;

  inline racer::math::point calculate_rear_axle_center(const racer::vehicle_configuration &cfg) const
  {
    auto rear_wheel_offset = racer::math::vector(-wheelbase_ / 2, 0).rotate(cfg.heading_angle());
    return cfg.location() + rear_wheel_offset;
  }
};

}  // namespace racer::following_strategies
