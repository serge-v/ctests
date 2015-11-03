cmake_minimum_required(VERSION 3.2)

#include_directories("/usr/local/include")

find_library(curl_LIBRARY NAMES curl PATHS "/usr/local/lib")
#find_library(ssl_LIBRARY NAMES ssl)
find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(common)

add_executable(curltest	curltest.c)
target_link_libraries(curltest ${curl_LIBRARY})

add_executable(rextest rextest.c)
add_executable(defines defines.c)
add_executable(cleanup-attr cleanup.c)

add_subdirectory(common)

add_executable(
	svhttpd
	svhttpd.c
)

target_link_libraries(svhttpd ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} svc)

add_executable(mailto mailto.c)
target_link_libraries(mailto ${curl_LIBRARY} svc)

add_custom_target(run
	COMMAND ./rextest.sh
	DEPENDS rextest
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
