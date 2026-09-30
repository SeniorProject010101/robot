


cmake -S . -B build
cmake --build build
./build/coreSystem

colcon build --symlink-install --base-paths robot_sim