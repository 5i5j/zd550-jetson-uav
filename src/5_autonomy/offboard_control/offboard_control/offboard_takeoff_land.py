#!/usr/bin/env python3
"""
Offboard mission for ZD550 Jetson UAV.

Flow:
  1. Wait for PX4 connection
  2. Arm
  3. Switch to OFFBOARD mode
  4. Takeoff to 5m
  5. Fly a square pattern (4 waypoints)
  6. Land
  7. Disarm

Uses uXRCE-DDS (PX4 <-> ROS2) topics from px4_msgs.
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, DurabilityPolicy

from px4_msgs.msg import (
    OffboardControlMode,
    TrajectorySetpoint,
    VehicleCommand,
    VehicleLocalPosition,
    VehicleStatus,
)

import time
from enum import Enum


class MissionState(Enum):
    WAIT_FOR_PX4 = 0
    ARMING = 1
    SETTING_OFFBOARD = 2
    TAKEOFF = 3
    MISSION = 4
    LANDING = 5
    DONE = 6


class OffboardMission(Node):
    def __init__(self):
        super().__init__('offboard_mission')

        # QoS profile required by PX4 uXRCE-DDS
        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
            history=HistoryPolicy.KEEP_LAST,
            depth=1,
        )

        # Publishers
        self.offboard_mode_pub = self.create_publisher(
            OffboardControlMode, '/fmu/in/offboard_control_mode', qos)
        self.trajectory_pub = self.create_publisher(
            TrajectorySetpoint, '/fmu/in/trajectory_setpoint', qos)
        self.vehicle_command_pub = self.create_publisher(
            VehicleCommand, '/fmu/in/vehicle_command', qos)

        # Subscribers
        self.local_pos_sub = self.create_subscription(
            VehicleLocalPosition, '/fmu/out/vehicle_local_position_v1',
            self.local_pos_callback, qos)
        self.vehicle_status_sub = self.create_subscription(
            VehicleStatus, '/fmu/out/vehicle_status_v4',
            self.vehicle_status_callback, qos)

        # State
        self.state = MissionState.WAIT_FOR_PX4
        self.local_pos = None
        self.vehicle_status = None
        self.takeoff_alt = -5.0  # NED: -5m = 5m above ground
        self.mission_waypoints = [
            (-0.5, -0.0, self.takeoff_alt),   # Hover at takeoff point
            (-5.0, -0.0, self.takeoff_alt),   # Forward
            (-5.0, -5.0, self.takeoff_alt),   # Left
            (-0.5, -5.0, self.takeoff_alt),   # Back
            (-0.5, -0.0, self.takeoff_alt),   # Return
        ]
        self.current_wp = 0
        self.wp_start_time = None
        self.offboard_setpoint_counter = 0
        self.state_start_time = time.time()

        # Main loop at 10 Hz (PX4 requires >2Hz for offboard)
        self.timer = self.create_timer(0.1, self.main_loop)

        self.get_logger().info('Offboard mission node started')

    # ---------------------------------------------------------------
    # Callbacks
    # ---------------------------------------------------------------
    def local_pos_callback(self, msg):
        self.local_pos = msg

    def vehicle_status_callback(self, msg):
        self.vehicle_status = msg

    # ---------------------------------------------------------------
    # Publish helpers
    # ---------------------------------------------------------------
    def timestamp(self):
        return int(self.get_clock().now().nanoseconds / 1000)

    def publish_offboard_control_mode(self):
        msg = OffboardControlMode()
        msg.timestamp = self.timestamp()
        msg.position = True
        msg.velocity = False
        msg.acceleration = False
        msg.attitude = False
        msg.body_rate = False
        self.offboard_mode_pub.publish(msg)

    def publish_trajectory_setpoint(self, x, y, z, yaw=0.0):
        msg = TrajectorySetpoint()
        msg.timestamp = self.timestamp()
        msg.position = [float(x), float(y), float(z)]
        msg.yaw = float(yaw)
        self.trajectory_pub.publish(msg)

    def publish_vehicle_command(self, command, **params):
        msg = VehicleCommand()
        msg.timestamp = self.timestamp()
        msg.command = command
        msg.param1 = params.get('param1', 0.0)
        msg.param2 = params.get('param2', 0.0)
        msg.param3 = params.get('param3', 0.0)
        msg.param4 = params.get('param4', 0.0)
        msg.param5 = params.get('param5', 0.0)
        msg.param6 = params.get('param6', 0.0)
        msg.param7 = params.get('param7', 0.0)
        msg.target_system = 1
        msg.target_component = 1
        msg.source_system = 1
        msg.source_component = 1
        msg.from_external = True
        self.vehicle_command_pub.publish(msg)

    # ---------------------------------------------------------------
    # Main state machine
    # ---------------------------------------------------------------
    def main_loop(self):
        # Always publish offboard mode + setpoint (PX4 requires continuous stream)
        self.publish_offboard_control_mode()

        if self.state == MissionState.WAIT_FOR_PX4:
            if self.vehicle_status is not None and self.local_pos is not None:
                self.get_logger().info('PX4 connected, starting arming sequence')
                self.state = MissionState.ARMING
                self.state_start_time = time.time()
            else:
                return

        elif self.state == MissionState.ARMING:
            # Send setpoint stream for ~1s before switching mode
            self.publish_trajectory_setpoint(0.0, 0.0, 0.0)
            if time.time() - self.state_start_time > 1.0:
                self.get_logger().info('Sending ARM command')
                self.publish_vehicle_command(
                    VehicleCommand.VEHICLE_CMD_COMPONENT_ARM_DISARM,
                    param1=1.0)  # 1 = arm
                self.state = MissionState.SETTING_OFFBOARD
                self.state_start_time = time.time()

        elif self.state == MissionState.SETTING_OFFBOARD:
            self.publish_trajectory_setpoint(0.0, 0.0, 0.0)
            if time.time() - self.state_start_time > 1.0:
                self.get_logger().info('Switching to OFFBOARD mode')
                self.publish_vehicle_command(
                    VehicleCommand.VEHICLE_CMD_DO_SET_MODE,
                    param1=1.0,  # base mode
                    param2=6.0)  # 6 = OFFBOARD
                self.state = MissionState.TAKEOFF
                self.state_start_time = time.time()

        elif self.state == MissionState.TAKEOFF:
            self.publish_trajectory_setpoint(0.0, 0.0, self.takeoff_alt)
            if time.time() - self.state_start_time > 8.0:
                self.get_logger().info('Takeoff complete, starting mission')
                self.state = MissionState.MISSION
                self.current_wp = 0
                self.wp_start_time = time.time()

        elif self.state == MissionState.MISSION:
            wp = self.mission_waypoints[self.current_wp]
            self.publish_trajectory_setpoint(*wp)

            if self.local_pos is not None:
                dx = self.local_pos.x - wp[0]
                dy = self.local_pos.y - wp[1]
                dz = self.local_pos.z - wp[2]
                dist = (dx*dx + dy*dy + dz*dz) ** 0.5

                if dist < 0.5 or (time.time() - self.wp_start_time > 8.0):
                    self.get_logger().info(
                        f'Waypoint {self.current_wp + 1}/{len(self.mission_waypoints)} reached '
                        f'(dist={dist:.2f}m)')
                    self.current_wp += 1
                    self.wp_start_time = time.time()

                    if self.current_wp >= len(self.mission_waypoints):
                        self.get_logger().info('Mission complete, landing')
                        self.state = MissionState.LANDING
                        self.state_start_time = time.time()

        elif self.state == MissionState.LANDING:
            self.get_logger().info('Sending LAND command')
            self.publish_vehicle_command(
                VehicleCommand.VEHICLE_CMD_NAV_LAND)
            self.state = MissionState.DONE
            self.state_start_time = time.time()

        elif self.state == MissionState.DONE:
            # Keep publishing setpoint for safety
            self.publish_trajectory_setpoint(0.0, 0.0, 0.0)
            if time.time() - self.state_start_time > 15.0:
                self.get_logger().info('Disarming and exiting')
                self.publish_vehicle_command(
                    VehicleCommand.VEHICLE_CMD_COMPONENT_ARM_DISARM,
                    param1=0.0)  # 0 = disarm
                raise SystemExit


def main(args=None):
    rclpy.init(args=args)
    node = OffboardMission()
    try:
        rclpy.spin(node)
    except SystemExit:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()