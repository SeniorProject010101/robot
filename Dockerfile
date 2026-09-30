# Tiny ROS 2 image — same image runs on Apple Silicon Mac and Raspberry Pi 4B

FROM ros:jazzy-ros-core

ENV DEBIAN_FRONTEND=noninteractive \
    RMW_IMPLEMENTATION=rmw_fastrtps_cpp \
    ROS_DOMAIN_ID=0

# ROS tools for the robot and its small Gazebo simulation.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    g++ cmake make \
    python3-colcon-common-extensions \
    ros-jazzy-ros-gz-sim \
    ros-jazzy-ros-gz-bridge \
    ros-jazzy-rviz2 \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-xacro \
    xvfb x11vnc fluxbox novnc python3-websockify \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /robot

RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc \
 && echo "[ -f /robot/install/setup.bash ] && source /robot/install/setup.bash" >> /root/.bashrc

# Virtual desktop for Gazebo/RViz when the host has no X display
COPY docker/entrypoint.sh /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

CMD ["bash"]
