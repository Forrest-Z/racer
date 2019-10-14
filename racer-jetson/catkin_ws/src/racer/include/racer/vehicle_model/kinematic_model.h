#ifndef KINEMATIC_MODEL_H_
#define KINEMATIC_MODEL_H_

#include <iostream>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>

#include "racer/vehicle_configuration.h"

#include "racer/action.h"
#include "racer/math.h"

#include "racer/vehicle_model/vehicle_chassis.h"
#include "racer/vehicle_model/base_model.h"
#include "racer/vehicle_model/steering_servo_model.h"
#include "racer/vehicle_model/motor_model.h"

namespace racer::vehicle_model::kinematic
{

struct state
{
private:
    vehicle_configuration configuration_;
    rpm motor_rpm_;
    racer::math::angle steering_angle_;

public:
    state()
        : configuration_{},
          motor_rpm_{0},
          steering_angle_{0}
    {
    }

    state(vehicle_configuration configuration)
        : state(configuration, 0, 0)
    {
    }

    state(vehicle_configuration configuration, rpm motor_rpm, racer::math::angle steering_angle)
        : configuration_(configuration), motor_rpm_(motor_rpm), steering_angle_(steering_angle)
    {
    }

    state(const state &s) = default;
    state &operator=(const state &s) = default;

    state(state &&s) = default;
    state &operator=(state &&s) = default;

    inline const vehicle_configuration &configuration() const { return configuration_; }
    inline const racer::math::point &position() const { return configuration_.location(); }
    inline const rpm &motor_rpm() const { return motor_rpm_; }
    inline const racer::math::angle &steering_angle() const { return steering_angle_; }

    inline bool operator==(const state &other) const
    {
        return configuration_ == other.configuration_ && motor_rpm_ == other.motor_rpm_ && steering_angle_ == other.steering_angle_;
    }

    inline bool operator!=(const state &other) const
    {
        return !(*this == other);
    }

    inline double distance_to(const state &other) const
    {
        return (configuration_.location() - other.configuration_.location()).length();
    }

    inline bool is_valid() const { return configuration_.is_valid(); }
};

class model : public vehicle_model<state>
{
public:
    const std::unique_ptr<vehicle_chassis> chassis;

    model(std::unique_ptr<vehicle_chassis> chassis)
        : chassis{std::move(chassis)}
    {
    }

public:
    state predict_next_state(const state &current, const action &input, const double dt) const override
    {
        rpm motor_rpm = chassis->motor->predict_next_state(current.motor_rpm(), input, dt);
        racer::math::angle steering_angle = chassis->steering_servo->predict_next_state(current.steering_angle(), input, dt);

        // the computed engine RPM is used immediately - not with a 1 step delay
        // - otherwise, the car could not start moving in the very first step (with the current implementation)

        double v = calculate_speed_with_no_slip_assumption(motor_rpm);
        double slip_angle = atan((chassis->distance_of_center_of_gravity_to_rear_axle / chassis->wheelbase) * tan(steering_angle));

        // just some renaming to make the equations look the same as in the thesis
        // the compiler will optimize this
        double theta = current.configuration().heading_angle();
        double delta = current.steering_angle();
        double beta = slip_angle;
        double L = chassis->wheelbase;

        vehicle_configuration translation_and_rotation{
            v * cos(theta + beta) * dt,
            v * sin(theta + beta) * dt,
            v * cos(beta) * tan(delta) / L * dt};

        return {current.configuration() + translation_and_rotation, motor_rpm, steering_angle};
    }

    double maximum_theoretical_speed() const override
    {
        return chassis->motor->max_rpm() * chassis->wheel_radius;
    }

    double calculate_speed_with_no_slip_assumption(const rpm &motor_rpm) const
    {
        return (motor_rpm.to_radians_per_second() / chassis->motor_to_wheel_gear_ratio) * chassis->wheel_radius;
    }
};

} // namespace racer::vehicle_model::kinematic

#endif