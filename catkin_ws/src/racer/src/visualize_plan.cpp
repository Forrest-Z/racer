#include <iostream>

#define _USE_MATH_DEFINES
#include <cmath>

#include <ros/ros.h>

#include <geometry_msgs/Point.h>
#include <racer_msgs/State.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

#include "racer/math.h"
#include "racer/vehicle/chassis.h"
#include "racer/vehicle/configuration.h"
#include "racer/vehicle/kinematic/model.h"
#include "racer/vehicle/motor_model.h"
#include "racer/vehicle/steering_servo_model.h"
#include "racer/vehicle/trajectory.h"

#include "racer_ros/utils.h"

std::optional<racer::vehicle::trajectory<racer::vehicle::kinematic::state>>
    trajectory;

void trajectory_callback(const racer_msgs::Trajectory::ConstPtr &msg) {
  const double time_step_s = 0.1;
  trajectory = racer_ros::msg_to_trajectory(*msg, time_step_s);
}

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "visualize_plan");
  ros::NodeHandle node("~");

  std::string trajectory_topic, state_topic, visualization_topic;
  node.param<std::string>("trajectory_topic", trajectory_topic,
                          "/racer/trajectory");
  node.param<std::string>("visualization_topic", visualization_topic,
                          "/racer/visualization/trajectory");

  auto trajectory_sub = node.subscribe<racer_msgs::Trajectory>(
      trajectory_topic, 1, trajectory_callback);
  auto visualization_pub = node.advertise<visualization_msgs::MarkerArray>(
      visualization_topic, 1, true);

  const auto model = std::make_shared<racer::vehicle::kinematic::model>(
      racer::vehicle::chassis::simulator());
  int seq = 0;

  double frequency = 25.0;
  ros::Rate rate{frequency};

  while (ros::ok()) {
    if (trajectory) {
      visualization_msgs::MarkerArray markers;

      for (const auto &step : trajectory->steps()) {
        visualization_msgs::Marker marker;
        marker.header.frame_id = "map";
        marker.id = seq;
        marker.header.seq = seq++;
        marker.header.stamp = ros::Time::now();
        marker.lifetime = ros::Duration{1 / frequency};
        marker.type = visualization_msgs::Marker::ARROW;
        marker.action = visualization_msgs::Marker::ADD;
        marker.ns = "trajectory";

        double speed_proportion =
            step.state().motor_rpm() / model->chassis->motor->max_rpm();
        marker.color.g = speed_proportion;
        marker.color.r = 1 - speed_proportion;
        marker.color.a = 1.0;

        marker.pose.position.x = step.state().position().x();
        marker.pose.position.y = step.state().position().y();

        marker.pose.orientation =
            tf::createQuaternionMsgFromYaw(step.state().cfg().heading_angle());

        marker.scale.x = 0.01 + 0.49 * speed_proportion;
        marker.scale.y = 0.05;
        marker.scale.z = 0.05;

        markers.markers.push_back(marker);
      }

      visualization_pub.publish(markers);
    }

    rate.sleep();
    ros::spinOnce();
  }

  return 0;
}
