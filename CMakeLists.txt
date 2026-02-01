cmake_minimum_required(VERSION 3.20)
project(edoyun_access2_server)

# Enable the generation of the compilation database (a core file required by clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set compilation options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

# Global C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(reactor)
add_subdirectory(business)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE reactor)
target_link_libraries(${PROJECT_NAME} PRIVATE business)
