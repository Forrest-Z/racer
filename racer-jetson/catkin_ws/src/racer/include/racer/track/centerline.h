#pragma once

#include <iostream>
#include <vector>
#include <assert.h>

#include "racer/vehicle_configuration.h"
#include "racer/occupancy_grid.h"
#include "racer/math.h"
#include "racer/sehs/space_exploration.h"

namespace racer::track
{

class centerline {
private:
    centerline(std::vector<racer::math::circle> circles, std::vector<racer::math::vector> normals, double length, double width)
        : circles_{circles}, normals_{normals}, length_{length}, width_{width}
    {
        assert(circles_.size() == normals_.size());
    }

public:
    struct coordinate {
        const double distance_along;
        const double displacement;
    };

public:
    static centerline find(
        const racer::vehicle_configuration &start,
        const std::shared_ptr<racer::occupancy_grid> grid,
        const std::vector<racer::math::point> &checkpoints
    ) {
        const double max_radius = 5.0; // meters
        const racer::math::angle left = start.heading_angle() + racer::math::angle::from_degrees(90);
        const racer::math::angle right = start.heading_angle() - racer::math::angle::from_degrees(90);

        const double distance_left = grid->find_distance(start.location(), left, max_radius);
        const double distance_right = grid->find_distance(start.location(), right, max_radius);
        const double radius = (distance_left + distance_right) / 2;

        const double shift_right = distance_right - radius;
        const racer::math::point center = start.location() + racer::math::vector(shift_right, right);
        const racer::vehicle_configuration adjusted_start(center, start.heading_angle());

        racer::sehs::space_exploration space_exploration{0.8 * radius, 1.5 * radius, 17};
        const auto circles = space_exploration.explore_grid(grid, adjusted_start, checkpoints);

        std::vector<racer::math::vector> normals;
        normals.reserve(circles.size());

        double length = 0;

        for (std::size_t i = 0; i < circles.size(); ++i) {
            std::size_t prev = i == 0 ? circles.size() - 1 : i - 1;
            std::size_t next = (i + 1) % circles.size();
            const auto direction = circles[next].center() - circles[prev].center();
            normals.push_back(direction.normal());
            length += circles[i].center().distance(circles[prev].center());
        }

        return centerline{circles, normals, length, 2 * radius};
    }

    const std::vector<racer::math::circle>& circles() const {
        return circles_;
    }

    double length() const {
        return length_;
    }

    double width() const {
        return width_;
    }

    racer::math::point calculate_grid_coordinates(const coordinate& coord) const {
        const auto pt = point_at(coord.distance_along);
        const auto shift = normal_at(coord.distance_along) * coord.displacement * width_;
        return pt + shift;
    }

    coordinate coordinate_of(racer::math::point pt) const {
        return coordinate{0.0, 0.0};
    }
private:
    const std::vector<racer::math::circle> circles_;
    const std::vector<racer::math::vector> normals_;
    const double length_, width_;

    racer::math::vector normal_at(double distance_along) const {
        std::size_t prev = prev_index(distance_along);
        std::size_t next = next_index(prev);
        double weight = prev_weight(distance_along, prev);

        return normals_[prev].interpolate_with(normals_[next], weight, 1 - weight);
    }

    racer::math::point point_at(double distance_along) const {
        std::size_t prev = prev_index(distance_along);
        std::size_t next = next_index(prev);
        double weight = prev_weight(distance_along, prev);

        return circles_[prev].center().interpolate_with(circles_[next].center(), weight, 1 - weight);
    }

    std::size_t prev_index(double distance_along) const {
        return std::size_t(std::floor(distance_along * double(circles_.size()))) % circles_.size();
    }

    std::size_t next_index(std::size_t prev) const {
        return (prev + 1) % circles_.size();
    }

    double prev_weight(double distance_along, std::size_t prev) const {
        double prev_distance = double(prev) / double(circles_.size());
        double one_step = 1.0 / double(circles_.size());
        return 1.0 - (distance_along - prev_distance) / one_step;
    }
};

}