cmake_minimum_required(VERSION 3.2)

#include_directories("/usr/local/include")

find_library(curl_LIBRARY NAMES curl PATHS "/usr/local/lib")
#find_library(ssl_LIBRARY NAMES ssl)
find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(curltest	curltest.c)
target_link_libraries(curltest ${curl_LIBRARY})

add_executable(rextest rextest.c)
add_executable(defines defines.c)
add_executable(cleanup-attr cleanup.c)

add_executable(
	svhttpd
	svhttpd.c
	../common/socket.c ../common/socket.h
	../common/http.c ../common/http.h
	../common/struct.c ../common/struct.h
	../common/crypt.c ../common/crypt.h
)

target_link_libraries(svhttpd ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

add_executable(mailto mailto.c ../common/net.c  ../common/struct.c)
target_link_libraries(mailto ${curl_LIBRARY})

add_custom_target(run
	COMMAND ./rextest.sh
	DEPENDS rextest
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
