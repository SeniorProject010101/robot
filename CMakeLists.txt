cmake_minimum_required(VERSION 3.8)
project(car_core)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)

add_executable(driver src/car_core/driver.cpp)
ament_target_dependencies(
  driver
  "rclcpp"
  "geometry_msgs"
)
target_link_libraries(driver lgpio)

add_executable(teleop src/car_core/teleop.cpp)
ament_target_dependencies(
  teleop
  "rclcpp"
  "geometry_msgs"
)

install(TARGETS driver teleop DESTINATION lib/${PROJECT_NAME})

ament_package()
