#!/usr/bin/env bash
cd "$(dirname "$0")"
docker compose up -d --build

if [ "${1:-}" = "build" ]; then
  docker compose exec ros bash build.sh
else
  docker compose exec ros bash
fi
