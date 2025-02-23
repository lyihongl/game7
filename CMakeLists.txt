cmake_minimum_required(VERSION 3.22.1)
project (game7 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_subdirectory(../external/raylib)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Ensure the standard is required
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

set(RAYLIB_BINARY_DIR "${CMAKE_BINARY_DIR}/raylib")
find_package (Eigen3 3.4 REQUIRED NO_MODULE)

file(GLOB SOURCES
    ${CMAKE_SOURCE_DIR}/*.cpp
)
file(GLOB HEADERS 
    ${CMAKE_SOURCE_DIR}/include/*.h
)

message(STATUS "Sources: ${SOURCES}")

# Add the raylib subdirectory and specify the binary directory
add_subdirectory(../external/raylib ${RAYLIB_BINARY_DIR})

add_executable(game7 ${SOURCES} ${HEADERS})

target_link_libraries(game7 raylib Eigen3::Eigen)# Specify a binary directory for Raylib (e.g., inside your build directory)
target_include_directories(game7 PUBLIC ${CMAKE_SOURCE_DIR}/include)
