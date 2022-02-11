cmake_minimum_required(VERSION 3.19)
project(streaming-project)

set(${PROJECT_NAME}_MAJOR_VERSION 1)
set(${PROJECT_NAME}_MINOR_VERSION 0)
set(${PROJECT_NAME}_PATCH_VERSION 0)

# We don't want compiler warnings from included libraries
set(CONAN_SYSTEM_INCLUDES ON)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TASKS)
# conan setup is done

if (CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-Wall -Wextra )
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../bin)

add_subdirectory(${PROJECT_SOURCE_DIR}/pipeline-interception)
add_subdirectory(${PROJECT_SOURCE_DIR}/multipipe)
add_subdirectory(${PROJECT_SOURCE_DIR}/stillimage)

