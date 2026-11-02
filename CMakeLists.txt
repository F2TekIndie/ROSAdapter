cmake_minimum_required(VERSION 3.16)

project(rosadapter VERSION 0.1 LANGUAGES CXX)

#
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)

#
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

set(ROSDATA_WITH_ROS ON)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../ROSData"
    "${CMAKE_CURRENT_BINARY_DIR}/rosdata"
)

add_executable(rosadapter
    src/main.cpp
    src/server.h src/server.cpp
)

ament_target_dependencies(rosadapter
    rclcpp std_msgs
)

target_link_libraries(rosadapter
    rosdata_ros
)

include(CTest)
if(BUILD_TESTING)
    find_package(sensor_msgs REQUIRED)
    find_package(nav_msgs REQUIRED)
    add_executable(rosdata_converter_test
        tests/converter.cpp
    )
    target_link_libraries(rosdata_converter_test rosdata_ros)
    ament_target_dependencies(rosdata_converter_test
        rclcpp std_msgs sensor_msgs nav_msgs rosidl_typesupport_introspection_cpp
    )
    add_test(NAME rosdata_converter COMMAND rosdata_converter_test)
    add_test(NAME rosdata_generator COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../ROSData/tools/test_generator.py")

    add_executable(rosadapter_server_test
        tests/server.cpp
        src/server.cpp
    )
    target_include_directories(rosadapter_server_test PRIVATE src)
    target_link_libraries(rosadapter_server_test rosdata_ros)
    ament_target_dependencies(rosadapter_server_test rclcpp sensor_msgs)
    add_test(NAME rosadapter_server COMMAND rosadapter_server_test)
    set_tests_properties(rosadapter_server PROPERTIES TIMEOUT 30 ENVIRONMENT "ROS_DOMAIN_ID=199")
endif()
