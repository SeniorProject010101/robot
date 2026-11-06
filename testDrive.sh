# forward for 2 seconds, then the watchdog stops it
docker exec robot-ros bash -lc 'source /opt/ros/jazzy/setup.bash && \
  timeout 2 ros2 topic pub -r 10 /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}}"'