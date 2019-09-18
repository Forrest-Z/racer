#include <iostream>
#include <chrono>

#include "standalone-experiments/input.h"
#include "standalone-experiments/plot.h"

#include "racer/math/primitives.h"
#include "racer/track_analysis.h"

void stop_stopwatch(std::string name, const std::chrono::time_point<std::chrono::steady_clock> &start)
{
  const auto end = std::chrono::steady_clock::now();
  std::cout << name << " took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
}

int main(int argc, char *argv[])
{
  std::cout << "Track analysis standalone experiment - The Racer project" << std::endl
            << std::endl;

  if (argc != 2)
  {
    std::cerr << "Exactly one argument is requried - path of a circuit definition file." << std::endl;
    return 1;
  }

  std::filesystem::path input_file_name = argv[1];

  const auto config = track_analysis_input::load(input_file_name);
  if (!config)
  {
    std::cerr << "Loading configuration from '" << input_file_name << "' failed." << std::endl;
    return 2;
  }

  // Step 1: Run just space exploration
  std::cout << "RUN space exploration" << std::endl;
  const auto se_start = std::chrono::steady_clock::now();
  racer::sehs::space_exploration se(*config->occupancy_grid, config->radius, 2 * config->radius, config->neighbor_circles);
  const auto circles = se.explore_grid(config->initial_position, config->checkpoints);
  stop_stopwatch("space exploration", se_start);

  // Step 2: Run the full track analysis
  // min_distance_between_waypoints
  std::cout << "RUN track analysis" << std::endl;
  const auto track_analysis_start = std::chrono::steady_clock::now();
  racer::track_analysis analysis(*config->occupancy_grid, config->min_distance_between_waypoints, config->max_distance_between_waypoints, config->neighbor_circles);
  const auto waypoints = analysis.find_corners(config->radius, config->initial_position, config->checkpoints, true);
  stop_stopwatch("track analysis", track_analysis_start);

  // this is just for visualization and it does not have to be stopwatched
  const auto track_analysis_raw_start = std::chrono::steady_clock::now();
  const auto raw_waypoints = analysis.find_corners(config->radius, config->initial_position, config->checkpoints, false);
  stop_stopwatch("track analysis without merging", track_analysis_raw_start);

  // This requires Linux or WSL+Xserver
  std::cout << "Show interactive plot" << std::endl;
  plot_track_analysis(*config, circles, raw_waypoints, waypoints);

  std::cout << "Done." << std::endl;
  return 0;
}