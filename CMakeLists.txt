cmake_minimum_required(VERSION 3.20)

project(terminal_emulator_vulkan_renderer)

find_package(Vulkan REQUIRED)
add_subdirectory(vulkan_helper)

add_executable(generate_spirv_reader_include generate_spirv_reader_include.cpp)
set_property(TARGET generate_spirv_reader_include PROPERTY CXX_STANDARD 23)
add_custom_command(COMMENT "Generating spirv_reader_os.hpp"
                    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/spirv_reader_os.hpp
                    COMMAND generate_spirv_reader_include ${CMAKE_CURRENT_BINARY_DIR}/include/spirv_reader_os.hpp
                    DEPENDS generate_spirv_reader_include)

add_library(vulkan_renderer
    vulkan_renderer.cpp
    vulkan_renderer.hpp
    spirv_reader.hpp
    vertex_renderer.hpp
    vulkan_utility.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/shader_path.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/spirv_reader_os.hpp
    ${CMAKE_BINARY_DIR}/shaders/vertex.spv
    ${CMAKE_BINARY_DIR}/shaders/geometry.spv
    ${CMAKE_BINARY_DIR}/shaders/fragment.spv
    ${CMAKE_BINARY_DIR}/shaders/mesh.spv
    ${CMAKE_BINARY_DIR}/shaders/task.spv
)
target_include_directories(
    vulkan_renderer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)

target_link_libraries(vulkan_renderer PUBLIC glfw Vulkan::Vulkan vulkan_helper)
set_property(TARGET vulkan_renderer PROPERTY CXX_STANDARD 23)
if(WIN32)
add_subdirectory(freetype)
target_link_libraries(vulkan_renderer PUBLIC freetype)
endif()
if(1)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)
target_link_libraries(vulkan_renderer PUBLIC freetype)
target_include_directories(vulkan_renderer PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif()

function(compile_glsl stage glsl_file spv_file)
add_custom_command(COMMENT "Compiling ${stage} shader"
                    OUTPUT ${spv_file}
                    COMMAND Vulkan::glslangValidator -V --target-env vulkan1.3 -S ${stage} -o ${spv_file}
                            ${glsl_file}
                    MAIN_DEPENDENCY ${glsl_file}
                    DEPENDS ${glsl_file} Vulkan::glslangValidator)
endfunction()
function(compile_glsl_help stage file_name_without_postfix)
	compile_glsl(${stage}
		${CMAKE_CURRENT_SOURCE_DIR}/${file_name_without_postfix}.glsl
		${CMAKE_BINARY_DIR}/shaders/${file_name_without_postfix}.spv
	)
    set(
        ${file_name_without_postfix}_shader_path
        "shaders/${file_name_without_postfix}.spv"
        PARENT_SCOPE
    )
endfunction()

compile_glsl_help(vert vertex)
compile_glsl_help(frag fragment)
compile_glsl_help(geom geometry)
compile_glsl_help(mesh mesh)
compile_glsl_help(task task)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/template/shader_path.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/shader_path.hpp
)
