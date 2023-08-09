cmake_minimum_required(VERSION 3.10)
project(calculate_depth_from_disparity)

# 调试模式
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)

include_directories(
    include
    ${OpenCV_INCLUDE_DIRS}
    )

add_executable(calculate_depth_from_disparity calculate_depth_from_disparity.cpp)
target_link_libraries(calculate_depth_from_disparity ${OpenCV_LIBS})