cmake_minimum_required(VERSION 3.2)
project(ctests C)

find_library(curl_LIBRARY NAMES curl PATHS "/usr/local/lib")
find_package(OpenSSL REQUIRED)
find_package(LibXml2 REQUIRED)

include(../../../w/common/macros.cmake)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(../../../w/)
include_directories(${CMAKE_BINARY_DIR})

gen_version_c()

add_executable(curltest	curltest.c)
target_link_libraries(curltest ${curl_LIBRARY})

add_executable(rextest rextest.c)
add_executable(defines defines.c)
add_executable(cleanup-attr cleanup.c)

add_subdirectory(../../../w/common "${CMAKE_BINARY_DIR}/common")

add_executable(
	svhttpd
	svhttpd.c
)

target_link_libraries(
	svhttpd
	${OPENSSL_SSL_LIBRARY} 
	${OPENSSL_CRYPTO_LIBRARY} 
	svc
)

generate_resources(drawpad.html GENERATED)

add_executable(
	draw-server
	draw-server.c
	drawpad.html.c
)

target_link_libraries(
	draw-server
	${OPENSSL_SSL_LIBRARY} 
	${OPENSSL_CRYPTO_LIBRARY} 
	svc
)

add_executable(mailto mailto.c)
target_link_libraries(mailto ${curl_LIBRARY} svc)

add_executable(ncurses-hello ncurses-hello.c)
target_link_libraries(ncurses-hello ncurses)

add_executable(fork-exec fork-exec.c)
add_executable(tag-exec tag-exec.c)
add_executable(load-mlist load-mlist.c)
