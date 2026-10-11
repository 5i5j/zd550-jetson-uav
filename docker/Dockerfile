# ROS2 Humble official image (aarch64 native; Jetson can run it directly)
# Note: we do not use l4t-base for now because the project does not require CUDA at this stage;
#       if GPU inference is needed later, switch to a CUDA-enabled ROS2 image.
FROM ros:humble

ENV DEBIAN_FRONTEND=noninteractive

# ---------------------------------------------------------------------------
# 1. Basic toolchain
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y \
    python3-colcon-common-extensions \
    python3-pip \
    git \
    nano \
    wget \
    curl \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# ---------------------------------------------------------------------------
# 2. ROS2 packages required for UAVs (not using the manipulator stack with Nav2/MoveIt)
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y \
    ros-humble-ros-base \
    ros-humble-tf2-tools \
    ros-humble-tf2-ros \
    ros-humble-robot-state-publisher \
    ros-humble-xacro \
    ros-humble-rviz2 \
    ros-humble-rmw-cyclonedds-cpp \
    ros-humble-cv-bridge \
    ros-humble-image-transport \
    && rm -rf /var/lib/apt/lists/*

# ---------------------------------------------------------------------------
# 3. Main path: uXRCE-DDS — PX4 <-> ROS2 native bridge
#    Build MicroXRCEAgent from source (official PX4 recommended method)
# ---------------------------------------------------------------------------
RUN git clone --depth 1 https://github.com/eProsima/Micro-XRCE-DDS-Agent.git /tmp/uxrce && \
    cd /tmp/uxrce && mkdir build && cd build && \
    cmake .. && make -j"$(nproc)" && make install && \
    ldconfig /usr/local/lib && \
    rm -rf /tmp/uxrce

# ---------------------------------------------------------------------------
# 4. Secondary path: MAVLink / MAVSDK — cross-flight-controller compatibility layer
#    - mavsdk 4.x wheels require a newer libstdc++ than ros:humble provides on Jetson
#      (undefined symbol: std::__atomic_futex_unsigned_base::_M_futex_wait_until).
#      Pin to 3.x, which is sufficient for offboard / telemetry / mission APIs.
#    - The package prints a FutureWarning about being renamed to 'mavsdk-grpc'.
#      That is expected and harmless for the pinned version.
#    - Reserved for future ArduPilot / DJI adaptation (not used deeply yet).
# ---------------------------------------------------------------------------
RUN pip3 install --no-cache-dir \
    "mavsdk<4" \
    pymavlink

# ---------------------------------------------------------------------------
# 5. Python dependencies: intentionally NOT installing numpy / opencv via pip.
#    ros:humble ships apt-managed python3-opencv (4.5.4) + numpy (1.21.5), and
#    cv_bridge is compiled against that NumPy 1.x ABI. Installing numpy 2.x via
#    pip shadows the system numpy and breaks cv_bridge at import time
#    ("_ARRAY_API not found"). ROS2 users should rely on the apt versions.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 6. DDS configuration (uXRCE-DDS uses CycloneDDS, consistent with ROS2)
# ---------------------------------------------------------------------------
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# ---------------------------------------------------------------------------
# 7. ROS2 environment variables
# ---------------------------------------------------------------------------
RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc

WORKDIR /workspace/zd550-jetson-uav

CMD ["bash"]