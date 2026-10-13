cmake_minimum_required(VERSION 3.8)
project(offboard_control_cpp)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# executable
add_executable(offboard_mission_cpp src/offboard_mission.cpp)
ament_target_dependencies(offboard_mission_cpp
  rclcpp
  px4_msgs
  std_msgs
)

# install
install(TARGETS
  offboard_mission_cpp
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
