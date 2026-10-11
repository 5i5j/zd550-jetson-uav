#!/usr/bin/env bash
# Launch the container for SITL simulation (no hardware needed).
# Placeholder: PX4 SITL + Gazebo integration will be wired in later.

set -euo pipefail

IMAGE_TAG="${IMAGE_TAG:-zd550-jetson:latest}"
CONTAINER_NAME="${CONTAINER_NAME:-zd550-sim}"

echo "[sim] Starting container: ${CONTAINER_NAME}"
echo "[sim] TODO: PX4 SITL + Gazebo bringup not yet implemented."

docker run --rm -it \
    --name "${CONTAINER_NAME}" \
    --network host \
    -e DISPLAY="${DISPLAY:-}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)":/workspace/zd550-jetson-uav \
    "${IMAGE_TAG}" bash
