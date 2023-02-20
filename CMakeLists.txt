cmake_minimum_required(VERSION 3.14)

project(vulkan-intro)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

find_package(Vulkan REQUIRED)
find_program(GLSLC glslc REQUIRED)

function(add_shader TARGET SHADER)
    set(src_path ${CMAKE_SOURCE_DIR}/${SHADER})
    set(out_path ${CMAKE_BINARY_DIR}/${SHADER}.spv)
    get_filename_component(out_dir ${out_path} DIRECTORY)
    file(MAKE_DIRECTORY ${out_dir})
    add_custom_target(
        ${TARGET}
        ALL
        COMMAND ${GLSLC} -o ${out_path} ${src_path}
        DEPENDS ${src_path}
    )
endfunction(add_shader)

add_shader(vertex_shader shader.vert)

add_definitions(-DBUILD_TYPE="${CMAKE_BUILD_TYPE}")

add_executable(main main.c)
target_link_libraries(main vulkan)
