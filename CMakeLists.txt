cmake_minimum_required(VERSION 3.9)
project(async)
set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

add_library(async SHARED async.cpp async.h)
add_executable(async_cli main.cpp)

set_target_properties(async PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
    # COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra;-fsanitize=thread"
)

target_link_libraries(async_cli
	async
	${CMAKE_THREAD_LIBS_INIT}
)


