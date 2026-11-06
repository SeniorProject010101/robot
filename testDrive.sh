#!/usr/bin/env bash
docker exec robot-ros bash -lc 'source /opt/ros/jazzy/setup.bash && \
  ros2 topic pub -w 1 -r 10 -t 20 /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}"'