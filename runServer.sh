#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

docker compose --project-directory "$script_dir" up -d --build
docker compose --project-directory "$script_dir" exec -T ros bash /robot/build.sh

exec docker compose --project-directory "$script_dir" exec -T ros bash -lc \
  'source /opt/ros/jazzy/setup.bash && source /robot/install/setup.bash && exec ros2 run car_core driver'
