cmake_minimum_required(VERSION 3.20)
project(Platformer3D VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---
include(FetchContent)

# GLFW
message(STATUS "Fetching GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
message(STATUS "Fetching GLM...")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

# vk-bootstrap
message(STATUS "Fetching vk-bootstrap...")
FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v0.7
)
FetchContent_MakeAvailable(vk-bootstrap)

# Vulkan Memory Allocator (VMA)
message(STATUS "Fetching VMA...")
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)
find_program(GLSLC_EXECUTABLE NAMES glslc HINTS ${Vulkan_bin})
find_program(GLSLANG_VALIDATOR NAMES glslangValidator)

if(GLSLC_EXECUTABLE)
    message(STATUS "Using glslc for shader compilation: ${GLSLC_EXECUTABLE}")
    set(SHADER_COMPILER ${GLSLC_EXECUTABLE})
    set(SHADER_FLAGS "")
elseif(GLSLANG_VALIDATOR)
    message(STATUS "Using glslangValidator for shader compilation: ${GLSLANG_VALIDATOR}")
    set(SHADER_COMPILER ${GLSLANG_VALIDATOR})
    set(SHADER_FLAGS "-V")
else()
    message(FATAL_ERROR "No shader compiler found (glslc or glslangValidator). Please install vulkan-sdk or glslang-tools.")
endif()

# Shader Compilation
set(SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/assets/shaders")
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

file(GLOB SHADER_SOURCES "${SHADER_SOURCE_DIR}/*.vert" "${SHADER_SOURCE_DIR}/*.frag")
set(SPIRV_SHADERS "")

foreach(source_file ${SHADER_SOURCES})
    get_filename_component(file_name ${source_file} NAME)
    set(output_file "${SHADER_BINARY_DIR}/${file_name}.spv")
    list(APPEND SPIRV_SHADERS ${output_file})

    add_custom_command(
        OUTPUT ${output_file}
        COMMAND ${SHADER_COMPILER} ${SHADER_FLAGS} ${source_file} -o ${output_file}
        DEPENDS ${source_file}
        COMMENT "Compiling shader ${file_name}..."
    )
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_SHADERS})


# --- Sources ---
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})

# --- Includes & Linking ---
target_include_directories(${PROJECT_NAME} PRIVATE src external)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glm::glm
    vk-bootstrap
    Vulkan::Vulkan
)
add_dependencies(${PROJECT_NAME} Shaders)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_VULKAN GLFW_INCLUDE_NONE)



# VMA is header-only but needs implementation config usually in one cpp file
# We will handle VMA implementation define in src/renderer/vk_mem_alloc.cpp later
target_include_directories(${PROJECT_NAME} PRIVATE ${VulkanMemoryAllocator_SOURCE_DIR}/include)

# Shaders (TODO: Add shader compilation step)
