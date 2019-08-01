#include "ros/ros.h"
#include <cmath>
#include <tf/transform_datatypes.h>

#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Path.h>

#include <dynamic_reconfigure/server.h>
#include <racer/PIDConfig.h>

#include "racer_msgs/Trajectory.h"
#include "racer_msgs/Waypoints.h"

#include "racing/vehicle_model/kinematic_bicycle_model.h"
#include "racing/vehicle_model/base_vehicle_model.h"
#include "racing/collision_detection/occupancy_grid_collision_detector.h"
#include "racing/following_strategies/dwa.h"
#include "racing/following_strategies/geometric_following_strategy.h"
#include "math/euler_method_integrator.h"
#include "Follower.h"

std::shared_ptr<racing::pid> pid;

void pid_config_callback(racer::PIDConfig &config, uint32_t level) {
  if (!pid) return;
  pid->reconfigure(config.kp, config.ki, config.kd, config.error_tolerance);
}

int main(int argc, char* argv[]) {
  ros::init(argc, argv, "following_node");
  ros::NodeHandle node;

  double cell_size;
  std::string odometry_topic, trajectory_topic, waypoints_topic, costmap_topic, driving_topic, visualization_topic;

  node.param<double>("double", cell_size, 0.05);

  node.param<std::string>("costmap_topic", costmap_topic, "/costmap");
  node.param<std::string>("odometry_topic", odometry_topic, "/pf/pose/odom");
  node.param<std::string>("trajectory_topic", trajectory_topic, "/racer/trajectory");
  node.param<std::string>("waypoints_topic", waypoints_topic, "/racer/waypoints");

  node.param<std::string>("driving_topic", driving_topic, "/racer/commands");
  node.param<std::string>("visualization_topic", visualization_topic, "/racer/visualization/dwa");

  double max_speed, acceleration;

  node.param<double>("vehicle_max_speed", max_speed, 3.0);
  node.param<double>("vehicle_acceleration", acceleration, 2.0);

  racing::vehicle vehicle(
    0.155, // cog_offset
    0.31, // wheelbase
    0.35, // safe width
    0.55, // safe length
    2.0 / 3.0 * M_PI, // steering speed (rad/s)
    1.0 / 6.0 * M_PI, // max steering angle (rad)
    max_speed, // speed (ms^-1)
    acceleration // acceleration (ms^-2)
  );

  std::string strategy;
  node.param<std::string>("strategy", strategy, "dwa");
  std::unique_ptr<racing::following_strategy> following_strategy;

  double integration_step_s, prediction_horizon_s;
  node.param<double>("integration_step_s", integration_step_s, 1.0 / 20.0);
  node.param<double>("prediction_horizon_s", prediction_horizon_s, 0.5);

  racing::kinematic_model::model model(std::make_unique<math::euler_method>(integration_step_s), vehicle);

  const int lookahead = int(ceil(prediction_horizon_s / integration_step_s));

  if (strategy == "dwa") {
    std::cout << "DWA strategy" << std::endl;
    const auto actions = racing::kinematic_model::action::create_actions(5, 15);
    const auto detector = racing::collision_detector::precalculate(120, vehicle, cell_size);
    
    double position_weight, heading_weight, velocity_weight, distance_to_obstacle_weight;
    node.param<double>("position_weight", position_weight, 30.0);
    node.param<double>("heading_weight", heading_weight, 20.0);
    node.param<double>("velocity_weight", velocity_weight, 10.0);
    node.param<double>("distance_to_obstacle_weight", distance_to_obstacle_weight, 5.0);

    const racing::trajectory_error_calculator error_calculator(
      position_weight,
      heading_weight,
      velocity_weight,
      1.0,
      distance_to_obstacle_weight,
      vehicle.radius() * 5
    );

    following_strategy = std::make_unique<racing::dwa>(
      lookahead,
      actions,
      model,
      *detector,
      error_calculator
    );
  } else if (strategy == "geometric") {
    std::cout << "Geometric strategy" << std::endl;
    double kp, ki, kd, error_tolerance;
    node.param<double>("pid_speed_kp", kp, 1.0);
    node.param<double>("pid_speed_ki", ki, 0.0);
    node.param<double>("pid_speed_kd", kd, 1.0);
    node.param<double>("pid_speed_error_tolerance", error_tolerance, 0.5);
    pid = std::make_shared<racing::pid>(kp, ki, kd, error_tolerance);

    double min_lookahead, lookahead_coef;
    node.param<double>("min_lookahead", min_lookahead, vehicle.wheelbase * 5.0);
    node.param<double>("speed_lookahead_coef", lookahead_coef, 2.0);
    auto pure_pursuit = std::make_shared<racing::pure_pursuit>(vehicle, min_lookahead, lookahead_coef);

    dynamic_reconfigure::Server<racer::PIDConfig> server;
    dynamic_reconfigure::Server<racer::PIDConfig>::CallbackType f;  
    f = boost::bind(&pid_config_callback, _1, _2);
    server.setCallback(f);
  
    following_strategy = std::make_unique<racing::geometric_following_strategy>(pid, pure_pursuit);
  } else {
    throw std::invalid_argument("Unsupported following strategy.");
  }

  Follower follower(std::move(following_strategy));
  
  ros::Subscriber costmap_sub = node.subscribe<nav_msgs::OccupancyGrid>(costmap_topic, 1, &Follower::costmap_observed, &follower);
  ros::Subscriber odometry_sub = node.subscribe<nav_msgs::Odometry>(odometry_topic, 1, &Follower::state_observed, &follower);
  ros::Subscriber trajectory_sub = node.subscribe<racer_msgs::Trajectory>(trajectory_topic, 1, &Follower::trajectory_observed, &follower);
  ros::Subscriber waypoints_sub = node.subscribe<racer_msgs::Waypoints>(waypoints_topic, 1, &Follower::waypoints_observed, &follower);

  ros::Publisher command_pub = node.advertise<geometry_msgs::Twist>(driving_topic, 1);
  ros::Publisher visualization_pub = node.advertise<nav_msgs::Path>(visualization_topic, 1, true);

  int frequency; // Hz
  node.param<int>("update_frequency_hz", frequency, 20);

  ros::Rate rate(frequency);

  while (ros::ok()) {
    if (follower.is_initialized()) {
      auto action = follower.select_driving_command();

      if (!action) {
        action = follower.stop();
        std::cout << "following node: STOP!" << std::endl;
      } else {
        std::cout << "following node: [throttle: " << action->throttle << ", steering angle: " << action->target_steering_angle << "]" << std::endl;
      }

      geometry_msgs::Twist msg;
      msg.linear.x = action->throttle;
      msg.angular.z = action->target_steering_angle;

      command_pub.publish(msg);

      if (visualization_pub.getNumSubscribers() > 0) {
        nav_msgs::Path vis_msg;
        vis_msg.header.frame_id = follower.frame_id;
        vis_msg.header.stamp = ros::Time::now();

        auto state = std::make_unique<racing::kinematic_model::state>(follower.last_known_state());
        for (int i = 0; i < lookahead; ++i) {
          geometry_msgs::PoseStamped segment;

          state = std::move(model.predict(*state, *action));

          segment.header.frame_id = follower.frame_id;
          segment.header.stamp = ros::Time::now();
          segment.pose.position.x = state->position.x;
          segment.pose.position.y = state->position.y;
          segment.pose.position.z = 0;
          segment.pose.orientation = tf::createQuaternionMsgFromYaw(state->position.heading_angle);

          vis_msg.poses.push_back(segment);
        }

        visualization_pub.publish(vis_msg);
      }
    }

    ros::spinOnce();
    rate.sleep();
  }

  return 0;
}
