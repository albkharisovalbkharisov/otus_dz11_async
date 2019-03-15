cmake_minimum_required(VERSION 3.9)
project(async VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER} DESCRIPTION "some sa")
set(CMAKE_CXX_STANDARD 14)

find_package(Threads)

add_library(async SHARED async.cpp async.h)
add_executable(async_cli main.cpp)

target_link_libraries(async_cli
	async
	${CMAKE_THREAD_LIBS_INIT}
)

set_target_properties(async PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER async.h
)

#set_target_properties(async PROPERTIES PUBLIC_HEADER async.h)

include(GNUInstallDirs)
install(TARGETS async 
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCVLUDEDIR})
#install(FILES ${CMAKE_BINARY_DIR}/async.pc
#	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT albkharisov@gmail.com)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

