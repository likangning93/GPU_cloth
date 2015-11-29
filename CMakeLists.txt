cmake_minimum_required(VERSION 3.0)

project(cis565_GPU_cloth)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set up include and lib paths
set(EXTERNAL "external")
include_directories("${EXTERNAL}/include")
include_directories("${EXTERNAL}/src")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(EXTERNAL_LIB_PATH "${EXTERNAL}/lib/osx")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(EXTERNAL_LIB_PATH "${EXTERNAL}/lib/linux" "/usr/lib64")
elseif(WIN32)
    set(EXTERNAL_LIB_PATH "${EXTERNAL}/lib/win")
endif()
link_directories(${EXTERNAL_LIB_PATH})
list(APPEND CMAKE_LIBRARY_PATH "${EXTERNAL_LIB_PATH}")

# Find up and set up core dependency libs

find_package(Threads REQUIRED)
set(GLFW_INCLUDE_DIR "${EXTERNAL}/include")
set(GLFW_LIBRARY_DIR "${CMAKE_LIBRARY_PATH}")
find_library(GLFW_LIBRARY "glfw3" HINTS "${GLFW_LIBRARY_DIR}")

set(GLEW_INCLUDE_DIR "${EXTERNAL}/include")
set(GLEW_LIBRARY_DIR "${CMAKE_LIBRARY_PATH}")
add_definitions(-DGLEW_STATIC)
find_package(GLEW)

find_package(OpenGL)

set(CORELIBS
    "${GLFW_LIBRARY}"
    "${OPENGL_LIBRARY}"
    "${GLEW_LIBRARY}"
    "${CMAKE_THREAD_LIBS_INIT}"
    )

# Enable C++11 for host code
set(CMAKE_CXX_STANDARD 11)

# OSX-specific hacks/fixes
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND CORELIBS "-framework IOKit")
    list(APPEND CORELIBS "-framework Cocoa")
    list(APPEND CORELIBS "-framework CoreVideo")
endif()

# Linux-specific hacks/fixes
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lX11 -lXxf86vm -lXrandr -lXi")
endif()

add_subdirectory(src)

add_executable(${CMAKE_PROJECT_NAME}
    "src/main.hpp"
    "src/main.cpp"
    )

target_link_libraries(${CMAKE_PROJECT_NAME}
    src
    ${CORELIBS}
    )

add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders      
    )

add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/meshes
        ${CMAKE_BINARY_DIR}/meshes      
    )
