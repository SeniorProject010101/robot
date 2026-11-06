FROM ros:jazzy-ros-core

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    g++ cmake make \
    python3-colcon-common-extensions \
    ros-jazzy-rclcpp ros-jazzy-geometry-msgs \
    liblgpio-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /robot
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc

CMD ["bash", "-l"]
