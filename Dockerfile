FROM ros:jazzy-ros-core

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    python3-colcon-common-extensions \
    ros-jazzy-rclcpp ros-jazzy-geometry-msgs \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /robot

CMD ["bash", "-l"]
