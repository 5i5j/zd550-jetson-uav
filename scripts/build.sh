#!/usr/bin/env bash
# Build the Docker image for zd550-jetson-uav.
# Usage: ./scripts/build.sh [tag]

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${1:-zd550-jetson:latest}"

echo "[build] Building Docker image: ${IMAGE_TAG}"
cd "${REPO_ROOT}/docker"
docker build -t "${IMAGE_TAG}" . 2>&1 | tee build.log
echo "[build] Done. Image: ${IMAGE_TAG}"
