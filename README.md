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
or Docker Engine.

**macOS:** the container has a built-in virtual desktop. Enable host networking
once in Docker Desktop (Settings → Resources → Network → *Enable host
networking*, then Apply & restart). After launching the sim, open
<http://localhost:6080/vnc.html?autoconnect=1&resize=remote> to see Gazebo and RViz.

**Linux:** allow the container to open windows on your display:

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

### Changing the sim without rebuilding

The sim package is installed with `--symlink-install`, so the installed files
point straight at `robot_sim/`. Build it once:

```bash
colcon build --symlink-install --base-paths robot_sim
source /robot/install/setup.bash   # only needed in the shell you built in
```

New shells in the container load `install/setup.bash` automatically. After that, edits to existing files in `description/`, `worlds/`, `launch/`
and `rviz/` need no rebuild. Gazebo and RViz only read them at startup, so
restart the sim to see a change:

```bash
# Ctrl+C the running sim, then:
ros2 launch robot_sim simulation.launch.py
```

Then reload <http://localhost:6080/vnc.html?autoconnect=1&resize=remote>.

Rebuild (the `colcon build` above) only when you add a new file to the
package. C++ code (`main.cpp`, `core/`) always needs `./docker.sh build`.

Example: to change the robot's colour, edit the `rgba` value (red, green,
blue, opacity, each 0 to 1) of the body material at the top of
`robot_sim/description/robot.urdf.xacro`, then restart the sim.

Check the robot model for mistakes before launching:

```bash
xacro /robot/robot_sim/description/robot.urdf.xacro > /tmp/robot.urdf && check_urdf /tmp/robot.urdf
```

**If something's weird**

- Mac and Pi can't see each other? In Docker Desktop, go to Settings -> Resources -> Network and turn on host networking.
- Want a fresh start? `docker compose down` and run `./docker.sh` again. This clears the container's `build/`, so run the `colcon build` above again.
- `Package 'robot_sim' not found`? Run the `colcon build` above, then `source /robot/install/setup.bash`.
- `ros: command not found` / `python: command not found`? The commands are `ros2` and `python3`.
- Running a launch file with `python3` does nothing. Use `ros2 launch robot_sim <file>.launch.py`.
- Sim shows an old version of the robot? Another container may be holding port 6080. Run `docker ps`, then stop the old one with `docker stop <name>`.
- Gazebo and RViz open but no robot appears? The model file has an error. Run the `check_urdf` command above.
