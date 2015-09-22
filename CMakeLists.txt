cmake_minimum_required(VERSION 2.6)

enable_testing()

set(CMAKE_BUILD_TYPE Debug)

include_directories("/usr/local/include")

find_library(curl_LIBRARY NAMES curl PATHS "/usr/local/lib")

add_executable(curltest	curltest.c)
target_link_libraries(curltest ${curl_LIBRARY})

add_executable(rextest rextest.c)
add_executable(defines defines.c)
add_executable(cleanup-attr cleanup.c)

add_executable(mailto mailto.c ../common/net.c  ../common/struct.c)
target_link_libraries(mailto ${curl_LIBRARY})

add_custom_target(run
	COMMAND ./rextest.sh
	DEPENDS rextest
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
