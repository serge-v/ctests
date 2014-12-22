cmake_minimum_required(VERSION 2.6)

enable_testing()

set(CMAKE_BUILD_TYPE Debug)

add_executable(curltest	curltest.c)
target_link_libraries(curltest curl)

add_executable(rextest rextest.c)
target_link_libraries(rextest)

add_executable(departures departures.c fetch.c stations.c)
target_link_libraries(departures curl)

add_executable(curlsmtp curlsmtp.c)
target_link_libraries(curlsmtp curl)

add_test(NAME val
COMMAND valgrind --leak-check=full --show-reachable=no --track-fds=yes --error-exitcode=1
${CMAKE_CURRENT_BINARY_DIR}/departures
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

