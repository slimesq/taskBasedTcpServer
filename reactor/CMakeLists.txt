file(GLOB REACTOR_SRC "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")
message("REACTOR_SRC = ${REACTOR_SRC}")

find_package(Threads REQUIRED)

add_library(reactor SHARED ${REACTOR_SRC})
target_include_directories(reactor PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_link_libraries(reactor PUBLIC Threads::Threads)