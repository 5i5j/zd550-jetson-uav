#!/usr/bin/env bash
# Launch the container for real-hardware bringup.
# Placeholder: requires Pixhawk connected via USB/UART.

set -euo pipefail

IMAGE_TAG="${IMAGE_TAG:-zd550-jetson:latest}"
CONTAINER_NAME="${CONTAINER_NAME:-zd550-hw}"

echo "[hw] Starting container: ${CONTAINER_NAME}"
echo "[hw] TODO: Pixhawk serial passthrough + uXRCE-DDS agent not yet wired in."

# Common device paths for Pixhawk on Jetson: /dev/ttyACM0, /dev/ttyUSB0
docker run --rm -it \
    --name "${CONTAINER_NAME}" \
    --network host \
    --privileged \
    -e DISPLAY="${DISPLAY:-}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v /dev:/dev \
    -v "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)":/workspace/zd550-jetson-uav \
    "${IMAGE_TAG}" bash
