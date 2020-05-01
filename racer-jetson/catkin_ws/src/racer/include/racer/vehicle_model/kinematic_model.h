#pragma once

#include <algorithm>
#include <iostream>

#define _USE_MATH_DEFINES
#include <cmath>

#include "racer/vehicle_configuration.h"

#include "racer/action.h"
#include "racer/math.h"

#include "racer/vehicle_model/base_model.h"
#include "racer/vehicle_model/hardware_motor_model.h"
#include "racer/vehicle_model/vehicle_chassis.h"

namespace racer::vehicle_model::kinematic {
struct state {
private:
  vehicle_configuration configuration_;
  rpm motor_rpm_;
  racer::math::angle steering_angle_;

public:
  state() : configuration_{}, motor_rpm_{0}, steering_angle_{0} {}

  state(vehicle_configuration configuration) : state(configuration, 0, 0) {}

  state(vehicle_configuration configuration, rpm motor_rpm,
        racer::math::angle steering_angle)
      : configuration_(configuration), motor_rpm_(motor_rpm),
        steering_angle_(steering_angle) {}

  state(const state &s) = default;
  state &operator=(const state &s) = default;

  state(state &&s) = default;
  state &operator=(state &&s) = default;

  inline const vehicle_configuration &configuration() const {
    return configuration_;
  }
  inline const racer::math::point &position() const {
    return configuration_.location();
  }
  inline const rpm &motor_rpm() const { return motor_rpm_; }
  inline const racer::math::angle &steering_angle() const {
    return steering_angle_;
  }

  inline bool operator==(const state &other) const {
    return configuration_ == other.configuration_ &&
           motor_rpm_ == other.motor_rpm_ &&
           steering_angle_ == other.steering_angle_;
  }

  inline bool operator!=(const state &other) const { return !(*this == other); }

  inline double distance_to(const state &other) const {
    return configuration_.location().distance(other.configuration_.location());
  }

  inline bool is_valid() const { return configuration_.is_valid(); }

  friend std::ostream &operator<<(std::ostream &os, const state &s) {
    return os << "(theta: " << s.steering_angle() << ", rpm: " << s.motor_rpm()
              << ", " << s.configuration_.location() << ")";
  }
};

class model : public vehicle_model<state> {
public:
  const std::shared_ptr<vehicle_chassis> chassis;

  model(std::shared_ptr<vehicle_chassis> chassis) : chassis{chassis} {}

public:
  state predict_next_state(const state &current, const action &input,
                           const double dt) const override {
    auto motor_rpm =
        chassis->motor->predict_next_state(current.motor_rpm(), input, dt);
    auto steering_angle = chassis->steering_servo->predict_next_state(
        current.steering_angle(), input, dt);

    // the computed engine RPM is used immediately - not with a 1 step delay
    // - otherwise, the car could not start moving in the very first step (with
    // the current implementation)

    double v = calculate_speed_with_no_slip_assumption(motor_rpm);

    // calculate the effective steering angle given the current speed of the
    // vehicle - the higher the speed, the lower the effective steering angle
    double speed_percentage = std::clamp(v / top_speed_, 0.0, 1.0);
    double delta = effective_steering_angle(speed_percentage, steering_angle);

    // just some renaming to make the equations look the same as in the thesis
    double theta = current.configuration().heading_angle();
    double beta = slip_angle(delta);
    double L = chassis->wheelbase;

    vehicle_configuration translation_and_rotation{
        v * cos(theta + beta) * dt, v * sin(theta + beta) * dt,
        v * cos(beta) * tan(delta) / L * dt};

    return {current.configuration() + translation_and_rotation, motor_rpm,
            steering_angle};
  }

  double maximum_theoretical_speed() const override { return top_speed_; }

  double calculate_speed_with_no_slip_assumption(const rpm &motor_rpm) const {
    return (double(motor_rpm) / top_rpm_) * top_speed_;
  }

  double speed_in_state(const state &state) const {
    return calculate_speed_with_no_slip_assumption(state.motor_rpm());
  }

private:
  double slip_angle(const angle steering_angle) const {
    return atan((chassis->distance_of_center_of_gravity_to_rear_axle /
                 chassis->wheelbase) *
                tan(steering_angle));
  }

  double effective_steering_angle(double speed, double steering) const {
    // double m, c;
    // if (speed < 0.25) {
    //   m = 1.074 * speed + 0.406;
    //   c = -0.582 * speed + 0.155;
    // } else {
    //   m = -0.086 * speed + 0.229;
    //   c = -0.036 * speed + 0.026;
    // }

    // return racer::math::sign(steering) * (m * std::abs(steering) + c);
    return steering * racer::math::angle::from_degrees(10.5);
  }

private:
  const double top_speed_{4.1}, top_rpm_{780.0};
};

} // namespace racer::vehicle_model::kinematic
