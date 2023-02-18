cmake_minimum_required(VERSION 3.14)

project(vulkan-intro)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

find_package(Vulkan REQUIRED)

add_executable(main main.c)
target_link_libraries(main vulkan)
