# ZD550 Jetson UAV

Autonomous UAV built on the **ZD550** frame,
with onboard compute on **Jetson Orin Nano**, flight control via **PX4**,
and **ROS2 Humble** autonomy using a **dual-link architecture**:
**uXRCE-DDS** for high-rate PX4↔ROS2 communication, and
**MAVLink / MAVSDK** for cross-autopilot compatibility.

![ROS2](https://img.shields.io/badge/ROS2-Humble-blue)
![PX4](https://img.shields.io/badge/PX4-Autopilot-orange)
![Jetson](https://img.shields.io/badge/Jetson-Orin%20Nano-green)
![Ubuntu](https://img.shields.io/badge/Ubuntu-22.04-purple)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

---

## ✨ Features

- **PX4 flight control** targeting the ZD550 frame
- **Jetson Orin Nano** onboard compute for perception and offboard control
- **Dual-link FC ↔ ROS2 architecture**:
  - **uXRCE-DDS** — native, high-rate PX4 ↔ ROS2 communication *(primary)*
  - **MAVLink / MAVSDK** — cross-autopilot compatibility layer *(secondary)*
- **Offboard control**: takeoff, landing, waypoint navigation, trajectory tracking
- **Sensor integration**: camera, IMU, GPS, optical flow
- **Full SITL simulation** with Gazebo for safe development
- **Dockerized** environment for reproducible builds

---

## 🛠 Tech Stack

| Layer | Technology |
|-------|------------|
| Flight Controller | PX4 Autopilot |
| Onboard Compute | NVIDIA Jetson Orin Nano |
| Middleware | ROS2 Humble |
| FC ↔ ROS2 Bridge | **uXRCE-DDS** (primary) · **MAVSDK / MAVLink** (secondary) |
| Robot Model | URDF / xacro |
| Autonomy | Offboard control, waypoint nav, obstacle avoidance |
| Simulation | PX4 SITL + Gazebo |
| Perception | OpenCV / deep learning (TBD) |
| Build | colcon, CMake, ament |
| Container | Docker |

### Why two links?

| Link | Use case | Why |
|------|----------|-----|
| **uXRCE-DDS** | High-rate control, state estimation, offboard trajectories | Native PX4 DDS bridge — low latency, no serialization overhead, official ROS2 path |
| **MAVLink / MAVSDK** | Multi-autopilot support, ground-station compatibility, backup link | Universal protocol across PX4, ArduPilot, DJI OSDK ecosystems |

The two are **complementary, not competing** — uXRCE-DDS for performance,
MAVLink for portability.

---

## 📁 Structure

```
1_flight_control/       PX4 params, airframe config, uXRCE-DDS bridge
2_hardware_interfaces/  uXRCE-DDS + MAVSDK bridges, sensor drivers
3_description/          ZD550 URDF/xacro, sensor mounts
4_bringup/              Launch files and parameter configs
5_autonomy/             Offboard control, perception, mission planner
6_simulation/           PX4 SITL + Gazebo worlds and bringup
docker/                 Containerized dev environment
docs/                   Images and detailed setup
scripts/                Build and run helpers
```

---

## 🚀 Quick Start

### Prerequisites

- NVIDIA Jetson Orin Nano (JetPack 6.x)
- ZD550 frame + Pixhawk flight controller (PX4 firmware)
- Ubuntu 22.04 (host or via Docker)

### 1. Clone

```bash
git clone https://github.com/5i5j/zd550-jetson-uav.git
cd zd550-jetson-uav
```

### 2. Build Docker image

```bash
cd docker
docker build -t zd550-jetson:latest .
```

### 3. Run simulation (no hardware needed)

```bash
./scripts/run_sim.sh
```

### 4. Run on real hardware

```bash
./scripts/run_hardware.sh
```

---

## 🗺 Roadmap

- [x] Dockerized ROS2 + uXRCE-DDS + MAVSDK environment
- [ ] ZD550 URDF model
- [ ] PX4 SITL + Gazebo integration
- [ ] **uXRCE-DDS bridge to ROS2** *(primary link — in progress)*
- [ ] Offboard waypoint control
- [ ] **MAVSDK / MAVLink bridge** *(secondary link, for multi-autopilot support)*
- [ ] ArduPilot compatibility
- [ ] Vision-based obstacle avoidance
- [ ] Autonomous mission planner
- [ ] Real flight validation

---

## 📄 License

MIT — see [LICENSE](LICENSE)

---

## 🙏 Acknowledgements

- [PX4 Autopilot](https://px4.io/)
- [NVIDIA Jetson](https://developer.nvidia.com/embedded/jetson)
- [ROS2](https://docs.ros.org/en/humble/)
- [eProsima Micro XRCE-DDS](https://github.com/eProsima/Micro-XRCE-DDS-Agent)
- [Fast DDS](https://fast-dds.docs.eprosima.com/)
- [MAVSDK](https://mavsdk.mavlink.io/)