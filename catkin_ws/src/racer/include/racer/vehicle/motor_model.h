#pragma once

#include <array>
#include <iostream>

#define _USE_MATH_DEFINES
#include <cmath>

#include "racer/vehicle/base_motor_model.h"

namespace racer::vehicle
{

class motor_model : public base_motor_model
{
private:
  const rpm max_rpm_;
  const std::array<double, 6> x_;

public:
  motor_model(const rpm max_rpm, const std::array<double, 6> x)
      : max_rpm_{max_rpm}, x_{x} {}

  static auto rc_beast()
  {
    return std::make_unique<motor_model>(
        15350.0,
        std::array<double, 6>{7.33016701e+02, 8.58626896e+02, 7.40739969e-01,
                              7.68248846e+01, 2.05190302e+02, 1.16584276e+00});
  }

  static auto simulator()
  {
    return std::make_unique<motor_model>(
        17500.0,
        std::array<double, 6>{7.33016701e+02, 8.58626896e+02, 7.40739969e-01,
                              7.68248846e+01, 2.05190302e+02, 1.16584276e+00});
  }

  rpm max_rpm() const { return max_rpm_; }

  rpm predict_next_state(const rpm &state, const action &action,
                          const double dt) const
  {
    const double normalized_drive_torque =
        drive_torque(normalize_rpm(state), action.throttle());
    const double normalized_load_torque =
        load_torque(normalize_rpm(state), action.target_steering_angle());
    const double normalized_rpm_change_rate =
        (normalized_drive_torque - normalized_load_torque) / x_[1];

    return std::max(0.0,
                    double(state) + normalized_rpm_change_rate * max_rpm_ * dt);
  }

  double normalize_rpm(const rpm raw_rpm) const
  {
    return raw_rpm / max_rpm_;
  }

private:
  constexpr double max_torque(const double current_rpm) const
  {
    // there should be "current_rpm / max_rpm", but "max_rpm" would be
    // normalized max RPM, which would be 1.0
    return (1 - current_rpm) * x_[0];
  }

  constexpr double drive_torque(const double current_rpm,
                                const double throttle) const
  {
    return max_torque(current_rpm) * throttle;
  }

  constexpr double load_torque(const double current_rpm,
                                const double steering_angle) const
  {
    return std::pow(current_rpm, x_[2]) *
            std::pow(x_[3] + x_[4] * std::abs(steering_angle), x_[5]);
  }
};

} // namespace racer::vehicle
