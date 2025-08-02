cmake_minimum_required(VERSION 3.24.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_subdirectory(event-mgr)

# 测试项目
project(xd-event-mgr-test)

add_executable(xd-event-mgr-test ./test/main.cpp ./test/events.hpp)
target_link_libraries(xd-event-mgr-test PRIVATE xd-event-mgr)
