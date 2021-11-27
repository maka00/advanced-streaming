cmake_minimum_required(VERSION 3.19)
project(multipipe)

set(SOURCES main.cpp)


find_package(Threads)
find_package(PkgConfig REQUIRED)
find_package(Boost COMPONENTS filesystem iostreams system program_options timer REQUIRED)
pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GLib REQUIRED IMPORTED_TARGET glib-2.0)

enable_testing()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-D_DEBUG=1)
endif ()
add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME}
        PROPERTIES
        DEBUG_POSTFIX "d"
        CXX_STANDARD 20
        )

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}

        PRIVATE
        PkgConfig::GStreamer
        PkgConfig::GLib

        ${BOOST_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}

        )
# CMake 3.14 onwards
target_link_directories(${PROJECT_NAME}
        PRIVATE
        ../lib
        )

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${CONAN_LIBS}
        Threads::Threads
        Boost::program_options
        Boost::filesystem
        Boost::timer
        PkgConfig::GStreamer
        PkgConfig::GLib
        libgstapp-1.0.so
        )
target_compile_features(${PROJECT_NAME}
        PUBLIC
        cxx_std_17
        )

target_compile_options(${PROJECT_NAME}
        PUBLIC
        PRIVATE
        -Wall -Wextra -pedantic
        )
