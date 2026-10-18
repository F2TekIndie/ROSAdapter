cmake_minimum_required(VERSION 3.16)

project(rosadapter VERSION 0.1 LANGUAGES CXX)

#
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(THIRD_PARTY_ROOT "/home/f2tek/SwDev" CACHE PATH "Root of third-party libraries")

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

add_executable(rosadapter
    src/main.cpp
)

target_include_directories(rosadapter PRIVATE
	/opt/ros/jazzy/include
)

target_link_directories(rosadapter PRIVATE
	/opt/ros/jazzy/lib
)

target_link_libraries(rosadapter PRIVATE
	rclcpp rcutils
        Boost::system
        Threads::Threads
)

set_target_properties(rosadapter PROPERTIES
    BUILD_RPATH "/opt/ros/jazzy/lib"
    INSTALL_RPATH "/opt/ros/jazzy/lib"
)

