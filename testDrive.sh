#!/usr/bin/env bash
cd "$(dirname "$(readlink -f "$0")")"
exec docker compose -f compose.yaml -f compose.pi.yaml exec ros bash -lc \
  'source /opt/ros/jazzy/setup.bash && source /robot/install/setup.bash && ros2 run car_core teleop'