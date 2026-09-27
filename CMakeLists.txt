cmake_minimum_required(VERSION 3.16)
project(event_horizon LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)

add_executable(event_horizon black_hole.cpp)
target_include_directories(event_horizon PRIVATE ${GLFW3_INCLUDE_DIRS})
target_link_directories(event_horizon PRIVATE ${GLFW3_LIBRARY_DIRS})
target_link_libraries(event_horizon PRIVATE ${GLFW3_LIBRARIES} OpenGL::GL)
target_compile_options(event_horizon PRIVATE -Wall -Wextra -Wpedantic)