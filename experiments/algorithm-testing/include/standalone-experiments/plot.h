#ifndef PLOT_H_
#define PLOT_H_

#include <iostream>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include "matplotlib-cpp/matplotlibcpp.h"
#pragma GCC diagnostic pop

#include "standalone-experiments/input.h"

#include "racer/math.h"
#include "racer/circuit.h"
#include "racer/trajectory.h"
#include "racer/vehicle_configuration.h"
#include "racer/vehicle_model/kinematic_model.h"

using namespace racer::vehicle_model;
namespace plt = matplotlibcpp;

void plot_vehicle_configuration(const racer::vehicle_configuration &position, const double length, const double cell_size)
{
    auto end = racer::math::vector(length / cell_size, 0).rotate(-position.heading_angle());

    std::vector<double> x, y, u, v;
    x.push_back(position.location().x() / cell_size);
    y.push_back(position.location().y() / cell_size);
    u.push_back(end.x());
    v.push_back(end.y());

    plt::quiver(x, y, u, v, {{"color", "green"}});
}

void plot_path_of_circles(const std::string &name, const std::vector<racer::math::circle> &circles, const std::string &format, const double cell_size)
{
    std::vector<double> circles_x, circles_y;
    for (const auto &circle : circles)
    {
        circles_x.push_back(circle.center().x() / cell_size);
        circles_y.push_back(circle.center().y() / cell_size);
    }
    plt::plot(circles_x, circles_y, format, {{"label", name}});
}

void plot_circles(const std::vector<racer::math::point> &points, const std::shared_ptr<racer::occupancy_grid> occupancy_grid, const double radius, unsigned char *img)
{
    img = new unsigned char[occupancy_grid->cols() * occupancy_grid->rows() * 4];
    const auto raw_grid = occupancy_grid->raw_data();
    for (std::size_t row = 0; row < (std::size_t)occupancy_grid->rows(); ++row)
    {
        for (std::size_t col = 0; col < (std::size_t)occupancy_grid->cols(); ++col)
        {
            racer::math::point c(col * occupancy_grid->cell_size(), row * occupancy_grid->cell_size());

            std::size_t number_of_circles = 0;
            for (const auto &p : points)
            {
                if (c.distance_sq(p) < std::pow(radius, 2))
                {
                    number_of_circles++;
                }
            }

            std::size_t index = row * occupancy_grid->cols() * 4 + col * 4;
            img[index] = 0;
            img[index + 1] = 255;
            img[index + 2] = 0;
            img[index + 3] = number_of_circles * 30;
        }
    }

    plt::imshow(img, occupancy_grid->rows(), occupancy_grid->cols(), 4);
}

void plot_waypoints(const std::shared_ptr<racer::circuit> circuit, unsigned char *img)
{
    img = new unsigned char[circuit->grid->cols() * circuit->grid->rows() * 4];
    const auto raw_grid = circuit->grid->raw_data();
    for (std::size_t row = 0; row < (std::size_t)circuit->grid->rows(); ++row)
    {
        for (std::size_t col = 0; col < (std::size_t)circuit->grid->cols(); ++col)
        {
            racer::math::point c(col * circuit->grid->cell_size(), row * circuit->grid->cell_size());

            std::size_t number_of_waypoints = 0;
            for (std::size_t i = 0; i < circuit->waypoints.size(); ++i)
            {
                if (circuit->passes_waypoint(c, i))
                {
                    ++number_of_waypoints;
                }
            }

            std::size_t index = row * circuit->grid->cols() * 4 + col * 4;
            img[index] = 0;
            img[index + 1] = 0;
            img[index + 2] = 255;
            img[index + 3] = number_of_waypoints * 60;
        }
    }

    plt::imshow(img, circuit->grid->rows(), circuit->grid->cols(), 4);
}

void plot_points(const std::string &name, const std::vector<racer::math::point> &points, const std::string &format, const double cell_size)
{
    std::vector<double> points_x, points_y;
    for (const auto &point : points)
    {
        points_x.push_back(point.x() / cell_size);
        points_y.push_back(point.y() / cell_size);
    }
    double size = cell_size * 200;
    plt::plot(points_x, points_y, format, {{"label", name}, {"markersize", std::to_string(size)}});
}

void plot_trajectory(const std::string &name, const racer::trajectory<kinematic::state> trajectory, const double cell_size)
{
    std::vector<racer::math::point> points;
    for (const auto &step : trajectory.steps())
    {
        points.push_back(step.state().position());
    }

    plot_points(name, points, "k.", cell_size);
    plot_points(name, points, "r-", cell_size);
}

void plot_grid(const std::shared_ptr<racer::occupancy_grid> occupancy_grid, unsigned char *img)
{
    img = new unsigned char[occupancy_grid->cols() * occupancy_grid->rows() * 4];
    const auto raw_grid = occupancy_grid->raw_data();
    for (std::size_t i = 0; i < raw_grid.size(); ++i)
    {
        std::size_t index = i * 4;
        img[index] = img[index + 1] = img[index + 2] = (unsigned char)(255 - raw_grid[i]); // make obstacles dark
        img[index + 3] = raw_grid[i] < 50 ? 255 : 50;                                      // alpha
    }

    plt::imshow(img, occupancy_grid->rows(), occupancy_grid->cols(), 4);
}

void plot_track_analysis(
    const track_analysis_input &config,
    const std::vector<racer::math::circle> &circles,
    const std::vector<racer::math::point> &raw_waypoints,
    const std::vector<racer::math::point> &waypoints)
{
    // plt::title("Corner Detection");
    plt::grid(true);
    plt::xlim(0, config.occupancy_grid->cols());
    plt::ylim(0, config.occupancy_grid->rows());
    plt::subplot(1, 1, 1);

    unsigned char *grid_img = nullptr;
    plot_grid(config.occupancy_grid, grid_img);
    plot_points("Check points", config.checkpoints, "bx", config.occupancy_grid->cell_size());

    plot_path_of_circles("Path", circles, "k-", config.occupancy_grid->cell_size());
    plot_points("Merged corner points", raw_waypoints, "ro", config.occupancy_grid->cell_size());

    unsigned char *circles_img = nullptr;
    plot_circles(waypoints, config.occupancy_grid, config.min_distance_between_waypoints, circles_img);
    plot_points("Corners", waypoints, "go", config.occupancy_grid->cell_size());
    plot_vehicle_configuration(config.initial_position, config.radius, config.occupancy_grid->cell_size());

    plt::legend();
    plt::show();

    delete[] grid_img;
    delete[] circles_img;
}

void plot_trajectory(
    const track_analysis_input &config,
    const racer::trajectory<kinematic::state> &trajectory,
    const std::shared_ptr<racer::vehicle_model::kinematic::model> vehicle,
    const std::shared_ptr<racer::circuit> circuit,
    const std::string name)
{
    std::vector<racer::math::point> rpm_points, steering_angle_points, speed_points;
    for (const auto &step : trajectory.steps())
    {
        rpm_points.emplace_back(step.timestamp(), step.state().motor_rpm() / vehicle->chassis->motor->max_rpm());
        steering_angle_points.emplace_back(step.timestamp(), step.state().steering_angle());
        speed_points.emplace_back(step.timestamp(), vehicle->calculate_speed_with_no_slip_assumption(step.state().motor_rpm()));
    }

    plt::title("Trajectory");

    plt::subplot(1, 1, 1);
    plt::grid(true);
    plt::xlim(0, config.occupancy_grid->cols());
    plt::ylim(0, config.occupancy_grid->rows());

    unsigned char *grid_img = nullptr;
    unsigned char *circles_img = nullptr;
    unsigned char *waypoints_img = nullptr;
    plot_grid(config.occupancy_grid, grid_img);
    plot_circles(circuit->waypoints, circuit->grid, circuit->waypoint_radius, circles_img);
    plot_waypoints(circuit, waypoints_img);

    plot_vehicle_configuration(config.initial_position, config.radius, config.occupancy_grid->cell_size());
    plot_trajectory("Found trajectory", trajectory, config.occupancy_grid->cell_size());

    plt::legend();

    std::stringstream trajectory_file_name;
    trajectory_file_name << name << "_trajectory.png";
    plt::save(trajectory_file_name.str());

    // actuators state profile
    plt::subplot(2, 1, 1);

    plot_points("motor RPM (normalized)", rpm_points, "r-", 1.0);
    plot_points("steering angle (normalized)", steering_angle_points, "b-", 1.0);
    plt::legend();

    // speed profile
    plt::subplot(2, 1, 2);

    plot_points("speed profile", speed_points, "r-", 1.0);
    plt::legend();

    std::stringstream actuators_file_name;
    actuators_file_name << name << "_actuators.png";
    plt::save(actuators_file_name.str());

    delete[] grid_img;
    delete[] circles_img;
    delete[] waypoints_img;
}

#endif
