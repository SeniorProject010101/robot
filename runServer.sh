#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$script_dir"

compose() {
  docker compose --project-directory "$script_dir" \
    -f compose.yaml "$@"
}

compose up -d
compose exec -T ros pkill -f 'car_core/driver' || true      # no duplicates
compose exec -T ros bash -lc 'source /opt/ros/jazzy/setup.bash && colcon build --symlink-install'

exec docker compose --project-directory "$script_dir" \
  -f compose.yaml exec -T ros bash -lc \
  'source /opt/ros/jazzy/setup.bash && source /robot/install/setup.bash && exec ros2 run car_core driver'