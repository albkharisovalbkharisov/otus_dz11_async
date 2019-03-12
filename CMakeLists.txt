cmake_minimum_required(VERSION 3.9)
project(async)
set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

add_library(async SHARED async.cpp async.h)
add_executable(async_cli main.cpp)
target_link_libraries(async_cli
	async
	${CMAKE_THREAD_LIBS_INIT}
)


