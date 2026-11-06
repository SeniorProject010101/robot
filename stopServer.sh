#!/usr/bin/env bash
cd "$(dirname "$0")"
docker compose -f compose.yaml -f compose.pi.yaml exec -T ros pkill -INT -f 'car_core/driver' || true