/**
 * Offboard mission node for ZD550 Jetson UAV (C++ version).
 *
 * Mirrors the Python version's functionality:
 *   1. Wait for PX4 connection
 *   2. Arm
 *   3. Switch to OFFBOARD mode
 *   4. Takeoff to 5m
 *   5. Fly a square pattern
 *   6. Land + disarm
 *
 * Uses uXRCE-DDS (PX4 <-> ROS2) topics from px4_msgs.
 */

#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "px4_msgs/msg/offboard_control_mode.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_local_position.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

using namespace std::chrono_literals;
using OffboardControlMode = px4_msgs::msg::OffboardControlMode;
using TrajectorySetpoint = px4_msgs::msg::TrajectorySetpoint;
using VehicleCommand = px4_msgs::msg::VehicleCommand;
using VehicleLocalPosition = px4_msgs::msg::VehicleLocalPosition;
using VehicleStatus = px4_msgs::msg::VehicleStatus;

enum class State {
    WAIT_FOR_PX4,
    ARMING,
    SETTING_OFFBOARD,
    TAKEOFF,
    MISSION,
    LANDING,
    DONE
};

class OffboardMission : public rclcpp::Node
{
public:
    OffboardMission() : Node("offboard_mission")
    {
        // QoS profile required by PX4 uXRCE-DDS
        auto qos = rclcpp::QoS(rclcpp::KeepLast(1))
                       .best_effort()
                       .transient_local();

        // Publishers
        offboard_mode_pub_ = create_publisher<OffboardControlMode>(
            "/fmu/in/offboard_control_mode", qos);
        trajectory_pub_ = create_publisher<TrajectorySetpoint>(
            "/fmu/in/trajectory_setpoint", qos);
        vehicle_command_pub_ = create_publisher<VehicleCommand>(
            "/fmu/in/vehicle_command", qos);

        // Subscribers
        local_pos_sub_ = create_subscription<VehicleLocalPosition>(
            "/fmu/out/vehicle_local_position_v1", qos,
            std::bind(&OffboardMission::local_pos_callback, this, std::placeholders::_1));
        vehicle_status_sub_ = create_subscription<VehicleStatus>(
            "/fmu/out/vehicle_status_v4", qos,
            std::bind(&OffboardMission::vehicle_status_callback, this, std::placeholders::_1));

        // Mission parameters
        takeoff_alt_ = -5.0f;
        mission_waypoints_ = {
            {0.0f, 0.0f, takeoff_alt_},
            {-5.0f, 0.0f, takeoff_alt_},
            {-5.0f, -5.0f, takeoff_alt_},
            {0.0f, -5.0f, takeoff_alt_},
            {0.0f, 0.0f, takeoff_alt_},
        };

        // Main loop at 10 Hz (PX4 requires >2Hz for offboard)
        timer_ = create_wall_timer(100ms, std::bind(&OffboardMission::main_loop, this));

        state_ = State::WAIT_FOR_PX4;
        state_start_time_ = now();
        current_wp_ = 0;
        wp_start_time_ = now();

        RCLCPP_INFO(get_logger(), "Offboard mission node started");
    }

private:
    // ---------------- Publishers/Subscribers ----------------
    rclcpp::Publisher<OffboardControlMode>::SharedPtr offboard_mode_pub_;
    rclcpp::Publisher<TrajectorySetpoint>::SharedPtr trajectory_pub_;
    rclcpp::Publisher<VehicleCommand>::SharedPtr vehicle_command_pub_;
    rclcpp::Subscription<VehicleLocalPosition>::SharedPtr local_pos_sub_;
    rclcpp::Subscription<VehicleStatus>::SharedPtr vehicle_status_sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    // ---------------- State ----------------
    State state_;
    VehicleLocalPosition::SharedPtr local_pos_;
    VehicleStatus::SharedPtr vehicle_status_;
    rclcpp::Time state_start_time_;
    rclcpp::Time wp_start_time_;

    float takeoff_alt_;
    std::vector<std::array<float, 3>> mission_waypoints_;
    size_t current_wp_;

    // ---------------- Callbacks ----------------
    void local_pos_callback(const VehicleLocalPosition::SharedPtr msg)
    {
        local_pos_ = msg;
    }

    void vehicle_status_callback(const VehicleStatus::SharedPtr msg)
    {
        vehicle_status_ = msg;
    }

    // ---------------- Publish helpers ----------------
    uint64_t timestamp_us()
    {
        return static_cast<uint64_t>(now().nanoseconds() / 1000);
    }

    void publish_offboard_control_mode()
    {
        OffboardControlMode msg{};
        msg.timestamp = timestamp_us();
        msg.position = true;
        msg.velocity = false;
        msg.acceleration = false;
        msg.attitude = false;
        msg.body_rate = false;
        offboard_mode_pub_->publish(msg);
    }

    void publish_trajectory_setpoint(float x, float y, float z, float yaw = 0.0f)
    {
        TrajectorySetpoint msg{};
        msg.timestamp = timestamp_us();
        msg.position = {x, y, z};
        msg.yaw = yaw;
        trajectory_pub_->publish(msg);
    }

    void publish_vehicle_command(uint16_t command,
                                  float p1 = 0.0f, float p2 = 0.0f,
                                  float p3 = 0.0f, float p4 = 0.0f,
                                  float p5 = 0.0f, float p6 = 0.0f,
                                  float p7 = 0.0f)
    {
        VehicleCommand msg{};
        msg.timestamp = timestamp_us();
        msg.command = command;
        msg.param1 = p1;
        msg.param2 = p2;
        msg.param3 = p3;
        msg.param4 = p4;
        msg.param5 = p5;
        msg.param6 = p6;
        msg.param7 = p7;
        msg.target_system = 1;
        msg.target_component = 1;
        msg.source_system = 1;
        msg.source_component = 1;
        msg.from_external = true;
        vehicle_command_pub_->publish(msg);
    }

    // ---------------- State machine ----------------
    void main_loop()
    {
        // Always publish offboard mode + setpoint
        publish_offboard_control_mode();

        switch (state_) {
        case State::WAIT_FOR_PX4:
            if (vehicle_status_ && local_pos_) {
                RCLCPP_INFO(get_logger(), "PX4 connected, starting arming sequence");
                state_ = State::ARMING;
                state_start_time_ = now();
            }
            break;

        case State::ARMING: {
            publish_trajectory_setpoint(0.0f, 0.0f, 0.0f);
            double dt = (now() - state_start_time_).seconds();
            if (dt > 1.0) {
                RCLCPP_INFO(get_logger(), "Sending ARM command");
                publish_vehicle_command(
                    VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 1.0f);
                state_ = State::SETTING_OFFBOARD;
                state_start_time_ = now();
            }
            break;
        }

        case State::SETTING_OFFBOARD: {
            publish_trajectory_setpoint(0.0f, 0.0f, 0.0f);
            double dt = (now() - state_start_time_).seconds();
            if (dt > 1.0) {
                RCLCPP_INFO(get_logger(), "Switching to OFFBOARD mode");
                publish_vehicle_command(
                    VehicleCommand::VEHICLE_CMD_DO_SET_MODE, 1.0f, 6.0f);
                state_ = State::TAKEOFF;
                state_start_time_ = now();
            }
            break;
        }

        case State::TAKEOFF: {
            publish_trajectory_setpoint(0.0f, 0.0f, takeoff_alt_);
            double dt = (now() - state_start_time_).seconds();
            if (dt > 8.0) {
                RCLCPP_INFO(get_logger(), "Takeoff complete, starting mission");
                state_ = State::MISSION;
                current_wp_ = 0;
                wp_start_time_ = now();
            }
            break;
        }

        case State::MISSION: {
            auto wp = mission_waypoints_[current_wp_];
            publish_trajectory_setpoint(wp[0], wp[1], wp[2]);

            if (local_pos_) {
                float dx = local_pos_->x - wp[0];
                float dy = local_pos_->y - wp[1];
                float dz = local_pos_->z - wp[2];
                float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                double elapsed = (now() - wp_start_time_).seconds();

                if (dist < 0.5f || elapsed > 8.0) {
                    RCLCPP_INFO(get_logger(),
                        "Waypoint %zu/%zu reached (dist=%.2fm)",
                        current_wp_ + 1, mission_waypoints_.size(), dist);
                    current_wp_++;
                    wp_start_time_ = now();
                    if (current_wp_ >= mission_waypoints_.size()) {
                        RCLCPP_INFO(get_logger(), "Mission complete, landing");
                        state_ = State::LANDING;
                        state_start_time_ = now();
                    }
                }
            }
            break;
        }

        case State::LANDING:
            RCLCPP_INFO(get_logger(), "Sending LAND command");
            publish_vehicle_command(VehicleCommand::VEHICLE_CMD_NAV_LAND);
            state_ = State::DONE;
            state_start_time_ = now();
            break;

        case State::DONE: {
            publish_trajectory_setpoint(0.0f, 0.0f, 0.0f);
            double dt = (now() - state_start_time_).seconds();
            if (dt > 15.0) {
                RCLCPP_INFO(get_logger(), "Disarming and exiting");
                publish_vehicle_command(
                    VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM, 0.0f);
                rclcpp::shutdown();
            }
            break;
        }
        }
    }
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<OffboardMission>());
    rclcpp::shutdown();
    return 0;
}
