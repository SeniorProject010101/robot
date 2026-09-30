This project contains the robot's core connection, logic, and simulation tools.


## Running it with Docker

We run everything in a tiny ROS 2 container so the Mac and the Pi act exactly the same.

**First time on a machine**

- Mac: install Docker Desktop and open it.
- Pi: `curl -fsSL https://get.docker.com | sh`

**Then just**

```
./docker.sh          # drops you into a shell with ROS ready
./docker.sh build    # builds and runs our code
```

Edit the code normally; the container sees the mounted project files.

## Simulation

The project includes a small Gazebo and RViz simulation. You need Docker Desktop
or Docker Engine. Linux hosts also need `xhost`; macOS hosts need XQuartz.

On Linux, allow the container to open a display:

```
xhost +local:docker
```

Build the project and simulation package:

```bash
./docker.sh build
```

Start Gazebo and RViz in the first terminal:

```bash
docker compose exec ros bash -lc 'source /opt/ros/jazzy/setup.bash && source /robot/install/setup.bash && ros2 launch robot_sim simulation.launch.py'
```

Gazebo will show the robot on a flat ground plane. RViz will show its model.
Drive it from a second terminal:

```bash
docker compose exec ros bash -lc 'source /opt/ros/jazzy/setup.bash && source /robot/install/setup.bash && ros2 topic pub --rate 10 /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.3}, angular: {z: 0.4}}"'
```

Stop the simulator with `Ctrl+C`, then stop the container:

```bash
docker compose down
```

The robot model lives in `robot_sim/description`. The world and launch files
are in `robot_sim/worlds` and `robot_sim/launch`.

**If something's weird**

- Mac and Pi can't see each other? In Docker Desktop, go to Settings -> Resources -> Network and turn on host networking.
- Want a fresh start? `docker compose down` and run `./docker.sh` again.
