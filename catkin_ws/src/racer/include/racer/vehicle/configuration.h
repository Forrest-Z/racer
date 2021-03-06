#pragma once

#include "racer/math.h"

namespace racer::vehicle
{

struct configuration
{
private:
    racer::math::point location_;
    racer::math::angle heading_angle_;
    bool is_valid_;

public:
    configuration(double x, double y, double heading_angle)
        : location_{x, y}, heading_angle_{heading_angle}, is_valid_{true}
    {
    }

    configuration(racer::math::point location, double heading_angle)
        : location_{location}, heading_angle_{heading_angle}, is_valid_{true}
    {
    }

    configuration()
        : location_{0, 0}, heading_angle_{0}, is_valid_{false}
    {
    }

    configuration(const configuration &copy_from) = default;
    configuration &operator=(const configuration &copy_from) = default;

    configuration(configuration &&pos) = default;
    configuration &operator=(configuration &&pos) = default;

    inline bool operator==(const configuration &other) const
    {
        return location_ == other.location_ && heading_angle_ == other.heading_angle_;
    }

    inline bool is_valid() const
    {
        return is_valid_;
    }

    configuration operator+(const configuration &other) const
    {
        return configuration(location_ + other.location_, heading_angle_ + other.heading_angle_);
    }

    inline const racer::math::point &location() const { return location_; }
    inline const racer::math::angle &heading_angle() const { return heading_angle_; }
};

} // namespace racer
