cmake_minimum_required(VERSION 3.5)
project($CMAKE_PROJECT_NAME)
add_definitions()
include_directories()
link_directories()
add_executable(rtclock rtclock.c)
target_link_libraries(rtclock "rt" )

