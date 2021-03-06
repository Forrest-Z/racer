#include <atomic>
#include <chrono>
#include <iostream>
#include <numeric>
#include <optional>
#include <thread>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

#include "standalone-experiments/input.h"
#include "standalone-experiments/output.h"
#include "standalone-experiments/plot.h"

#include "racer/vehicle/action.h"
#include "racer/math.h"
#include "racer/space_exploration/space_exploration.h"
#include "racer/track/collision_detection.h"
#include "racer/vehicle/trajectory.h"
#include "racer/vehicle/kinematic/model.h"
#include "racer/vehicle/chassis.h"

#include "racer/astar/search.h"
#include "racer/astar/discretized/search_problem.h"
#include "racer/astar/hybrid_astar/discretization.h"
#include "racer/astar/sehs/discretization.h"

using state = racer::vehicle::kinematic::state;
using kinematic_model = racer::vehicle::kinematic::model;
using trajectory = racer::vehicle::trajectory<state>;
using search_result = racer::astar::search_result<state>;

using sehs_discrete_state = racer::astar::sehs::discrete_state;
using sehs_discretization = racer::astar::sehs::discretization;

using hybrid_astar_discrete_state = racer::astar::hybrid_astar::discrete_state;
using hybrid_astar_discretization = racer::astar::hybrid_astar::discretization;

std::unique_ptr<
    racer::astar::discretized::base_discretization<hybrid_astar_discrete_state, state>>
create_hybrid_astar_discretization(
    const racer::vehicle::chassis &vehicle,
    const double cell_size, const std::size_t heading_angle_bins,
    const std::size_t motor_rpm_bins)
{
  return std::make_unique<hybrid_astar_discretization>(
      cell_size, cell_size, 2 * M_PI / double(heading_angle_bins),
      vehicle.motor->max_rpm() / double(motor_rpm_bins));
}

std::unique_ptr<racer::astar::discretized::base_discretization<sehs_discrete_state, state>>
create_sehs_discretization(
    const racer::vehicle::chassis &vehicle,
    const std::shared_ptr<racer::track::occupancy_grid> occupancy_grid,
    const racer::vehicle::configuration start,
    const std::vector<racer::math::point> waypoints,
    const std::size_t heading_angle_bins, const std::size_t motor_rpm_bins)
{
  racer::space_exploration::space_exploration exploration{vehicle.radius(),
                                             5.0 * vehicle.radius(), 16};
  const auto path_of_circles =
      exploration.explore_grid(occupancy_grid, start, waypoints);
  if (path_of_circles.empty())
  {
    return nullptr;
  }

  return std::make_unique<racer::astar::sehs::discretization>(
      path_of_circles, 2 * M_PI / double(heading_angle_bins),
      vehicle.motor->max_rpm() / double(motor_rpm_bins));
}

template <typename DiscreteState>
output::planning::benchmark_result measure_search(
    std::shared_ptr<
        racer::astar::discretized::search_problem<DiscreteState, state>>
        problem,
    std::chrono::milliseconds time_limit)
{
  std::optional<search_result> result = {};
  std::chrono::milliseconds elapsed_time;

  const auto start = std::chrono::steady_clock::now();
  std::atomic<bool> terminate = false;

  std::thread work_thread([&]() {
    result = racer::astar::search<DiscreteState, state>(std::move(problem),
                                                        terminate);
    const auto end = std::chrono::steady_clock::now();
    elapsed_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  });

  bool exceeded_time_limit = false;
  do
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    exceeded_time_limit =
        !result && std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - start) > time_limit;
  } while (!result && !exceeded_time_limit);

  terminate = true;
  work_thread.join();

  assert(result.has_value());

  return {*result, elapsed_time, exceeded_time_limit};
}

template <typename DiscreteState>
void run_benchmark_for(
    const std::string algorithm,
    const std::shared_ptr<kinematic_model> vehicle_model,
    const track_analysis_input &config,
    const std::shared_ptr<racer::track::circuit> circuit,
    const std::shared_ptr<racer::track::collision_detection> collision_detector,
    const std::shared_ptr<racer::astar::discretized::base_discretization<DiscreteState, state>>
        state_discretization,
    const std::vector<racer::vehicle::action> actions,
    const racer::vehicle::configuration initial_config, const std::size_t start,
    const std::size_t lookahead, const double time_step_s,
    const std::size_t repetitions, const std::chrono::milliseconds time_limit,
    const bool plot)
{
  const auto initial_state = state{initial_config, 0, 0};

  const std::shared_ptr<racer::track::circuit> shifted_circut =
      circuit->for_waypoint_subset(start, lookahead);

  std::vector<double> measurement_times;
  measurement_times.reserve(repetitions);

  std::unique_ptr<output::planning::benchmark_result> measurement_sample;
  bool unsuccessful = false;

  for (std::size_t i = 0; i < repetitions; ++i)
  {
    auto problem = std::make_shared<
        racer::astar::discretized::search_problem<DiscreteState, state>>(
        initial_state, time_step_s, actions, state_discretization,
        vehicle_model, shifted_circut, collision_detector);

    const auto measurement = measure_search<DiscreteState>(problem, time_limit);
    if (!measurement_sample)
    {
      measurement_sample =
          std::make_unique<output::planning::benchmark_result>(measurement);
    }

    if (measurement.exceeded_time_limit)
    {
      unsuccessful = true;
      break;
    }

    measurement_times.push_back(measurement.computation_time.count());
  }

  if (unsuccessful)
  {
    output::planning::print_unsuccessful_result(
        algorithm, config.name, actions.size(), start, lookahead, time_step_s,
        state_discretization->description(), *measurement_sample, repetitions,
        time_limit.count());

    return;
  }

  const auto total = std::accumulate(std::begin(measurement_times),
                                     std::end(measurement_times), 0.0);
  const auto mean = total / double(measurement_times.size());

  const auto sum_of_squares = std::inner_product(
      std::begin(measurement_times), std::end(measurement_times),
      std::begin(measurement_times), 0.0);
  const auto variance =
      sum_of_squares / double(measurement_times.size()) - mean * mean;

  output::planning::print_result(
      algorithm, config.name, actions.size(), start, lookahead, time_step_s,
      state_discretization->description(), *measurement_sample,
      double(measurement_times.size()) / double(repetitions), repetitions, mean,
      variance);

  if (plot && measurement_sample->result.was_successful())
  {
    const auto experiment_name = output::planning::experiment_name(
        algorithm, config.name, actions.size(), start, lookahead, time_step_s,
        state_discretization->description());

    plot_trajectory(config, initial_config,
                    measurement_sample->result.found_trajectory, vehicle_model,
                    shifted_circut, experiment_name);
  }
}

void test_full_circuit_search(
    const std::vector<std::shared_ptr<track_analysis_input>> configs,
    const std::size_t repetitions, const std::chrono::milliseconds time_limit,
    const bool plot)
{
  std::shared_ptr<racer::vehicle::chassis> vehicle =
      racer::vehicle::chassis::simulator();
  const auto vehicle_model = std::make_shared<kinematic_model>(vehicle);

  const std::vector<std::size_t> steering{11, 21, 31};
  const std::vector<std::size_t> throttle{5, 9, 21};
  const std::vector<std::size_t> heading_angles{18, 36, 52};
  const std::vector<std::size_t> motor_rpms{20, 40, 80};
  const std::vector<double> cell_size_coefficients{4, 5};
  const std::vector<double> frequencies{25.0};

  for (const auto &config : configs)
  {
    std::vector<racer::math::point> final_check_points{
        config->checkpoints.begin(), config->checkpoints.end()};
    final_check_points.push_back(
        config->initial_position.location()); // back to the start

    // prepare circuit
    const auto centerline = racer::track::centerline::find(
        config->initial_position, config->occupancy_grid, final_check_points);
    if (centerline.empty())
    {
      std::cout << "Finding centerline failed for " << config->name
                << std::endl;
      continue;
    }
    racer::track::analysis analysis(centerline.width());
    const auto raw_waypoints = analysis.find_pivot_points(
        centerline.points(), config->occupancy_grid);
    auto sharp_turns =
        analysis.remove_insignificant_turns(raw_waypoints);

    // Insert the starting position to the list of waypoints to make sure that
    // we have some waypoint very close to the starting position.
    sharp_turns.push_back(config->initial_position.location());

    auto waypoints = analysis.merge_close(sharp_turns);
    const auto circuit = std::make_shared<racer::track::circuit>(
        waypoints, centerline.width(), config->occupancy_grid);

    const auto collision_detector =
        std::make_shared<racer::track::collision_detection>(
            config->occupancy_grid, vehicle, 72, 0.0);

    if (!circuit)
    {
      std::cerr
          << "Track analysis failed for " << config->name
          << " and it cannot be used for benchmarking (for vehicle radius of "
          << vehicle->radius() << "m)." << std::endl;
      return;
    }

    for (const auto heading_angle_bins : heading_angles)
      for (const auto motor_rpm_bins : motor_rpms)
        for (const auto frequency : frequencies)
          for (const auto t : throttle)
            for (const auto s : steering)
            {
              const std::size_t lookahead = waypoints.size();
              const auto actions = racer::vehicle::action::create_actions(t, s, 0.0, 1.0, -1.0, 1.0);

              const double time_step_s = 1.0 / frequency;

              for (const auto cell_size_coefficient : cell_size_coefficients)
              {
                auto hybrid_astar = create_hybrid_astar_discretization(
                    *vehicle, cell_size_coefficient * vehicle->radius(),
                    heading_angle_bins, motor_rpm_bins);
                run_benchmark_for<hybrid_astar_discrete_state>(
                    "hybrid_astar", vehicle_model, *config, circuit,
                    collision_detector, std::move(hybrid_astar), actions,
                    config->initial_position, 0, lookahead, time_step_s,
                    repetitions, time_limit, plot);
              }

              auto sehs = create_sehs_discretization(
                  *vehicle, config->occupancy_grid, config->initial_position,
                  config->checkpoints, heading_angle_bins, motor_rpm_bins);

              if (!sehs)
              {
                std::cerr << "SE failed and HS will be skipped." << std::endl;
                continue;
              }

              run_benchmark_for<sehs_discrete_state>(
                  "sehs", vehicle_model, *config, circuit, collision_detector,
                  std::move(sehs), actions, config->initial_position, 0,
                  lookahead, time_step_s, repetitions, time_limit, plot);
            }
  }
}

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cout
        << "Usage: " << argv[0]
        << " <number of repetitions> <time limit for one search in "
           "milliseconds> <circuit definition 1> [<circuit definition 2> ...]"
        << std::endl;
    return 1;
  }

  const std::size_t repetitions = static_cast<std::size_t>(atoi(argv[1]));
  const long long milliseconds = static_cast<long long>(atoi(argv[2]));
  const auto time_limit = std::chrono::milliseconds{milliseconds};
  const auto plot = true;

  const auto maybe_configs = track_analysis_input::load(argc - 3, argv + 3);
  if (!maybe_configs)
  {
    return 2;
  }

  output::planning::print_csv_header();
  test_full_circuit_search(*maybe_configs, repetitions, time_limit, plot);
}
