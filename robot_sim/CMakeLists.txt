cmake_minimum_required(VERSION 3.16)
project(robot_sim)

find_package(ament_cmake REQUIRED)

install(DIRECTORY description launch rviz worlds
  DESTINATION share/${PROJECT_NAME}
)

ament_package()