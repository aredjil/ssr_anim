cmake_minimum_required(VERSION 3.10)
project(ssr_animation.x)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()

add_executable(ssr_animation.x main.cpp)
target_link_libraries(ssr_animation.x sfml-graphics sfml-window sfml-system box2d)
