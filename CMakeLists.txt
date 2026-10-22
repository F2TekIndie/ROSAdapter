cmake_minimum_required(VERSION 3.16)

project(rosadapter VERSION 0.1 LANGUAGES CXX)

#
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(THIRD_PARTY_ROOT "/home/f2tek/SwDev" CACHE PATH "Root of third-party libraries")

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../ROSData"
    "${CMAKE_CURRENT_BINARY_DIR}/rosdata"
)

add_executable(rosadapter
    src/main.cpp
    src/configuration.h src/configuration.cpp
    src/cmdServer.h src/cmdServer.cpp
    src/server.h src/server.cpp
)
ament_target_dependencies(rosadapter
    rclcpp std_msgs
)

target_link_libraries(rosadapter
  Boost::system
  Threads::Threads
  rosdata
)

target_include_directories(rosadapter PRIVATE
	/opt/ros/jazzy/include/rcl
    /opt/ros/jazzy/include/rmw
    /opt/ros/jazzy/include/rcl_interfaces
    /opt/ros/jazzy/include/rclcpp
    /opt/ros/jazzy/include/rcutils
    /opt/ros/jazzy/include/rcpputils
    /opt/ros/jazzy/include/rosidl_runtime_cpp
    /opt/ros/jazzy/include/rosidl_runtime_c
    /opt/ros/jazzy/include/rosidl_typesupport_interface
    /opt/ros/jazzy/include/rosidl_dynamic_typesupport
    /opt/ros/jazzy/include/rosidl_typesupport_introspection_cpp
    /opt/ros/jazzy/include/type_description_interfaces
    /opt/ros/jazzy/include/service_msgs
    /opt/ros/jazzy/include/builtin_interfaces
    /opt/ros/jazzy/include/rcl_yaml_param_parser
    /opt/ros/jazzy/include/tracetools
    /opt/ros/jazzy/include/libstatistics_collector
    /opt/ros/jazzy/include/statistics_msgs
    /opt/ros/jazzy/include/std_msgs
)

target_link_directories(rosadapter PRIVATE
	/opt/ros/jazzy/lib
)

set_target_properties(rosadapter PROPERTIES
    BUILD_RPATH "/opt/ros/jazzy/lib"
    INSTALL_RPATH "/opt/ros/jazzy/lib"
)
