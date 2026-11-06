# Robot

Simple ROS 2 node for the car.

Build it with Docker:

```bash
./docker.sh build
```

Run the driver:

```bash
./runServer.sh
```

`runServer.sh` is non-interactive so it can be used as a systemd service.

Run it:

```bash
docker compose exec ros bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run car_core driver
```
