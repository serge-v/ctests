cmake_minimum_required(VERSION 2.6)

enable_testing()

set(CMAKE_BUILD_TYPE Debug)

include_directories("/usr/local/include")

find_library(curl_LIBRARY NAMES curl PATHS "/usr/local/lib")

add_executable(curltest	curltest.c)
target_link_libraries(curltest ${curl_LIBRARY})

add_executable(rextest rextest.c)
target_link_libraries(rextest)

add_executable(departures departures.c stations.c net.c)
target_link_libraries(departures ${curl_LIBRARY})

add_executable(mailto mailto.c net.c)
target_link_libraries(mailto ${curl_LIBRARY})

add_custom_target(run
COMMAND ./rextest.sh
DEPENDS rextest
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(NAME val
COMMAND valgrind --leak-check=full --show-reachable=no --track-fds=yes --error-exitcode=1
${CMAKE_CURRENT_BINARY_DIR}/departures
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

