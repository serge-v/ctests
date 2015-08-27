# Serge Voilokov, 2015
# cmake file for building weather tool.
#
#
# Usage:
# cd ~/b/weatherb
# cmake ~/src/xtree/ctests/weather/

cmake_minimum_required(VERSION 2.6)

find_library(ssl_LIBRARY NAMES ssl)
find_library(crypto_LIBRARY NAMES crypto)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(/opt/ports/dev/include/libxml2/)

add_executable(
	weather
	parse-dwml.c
	../common/net.c ../common/net.h
	../common/xml.c ../common/xml.h
	../common/struct.c ../common/struct.h
	version.c version.h
)

add_custom_command(
	OUTPUT "version.c" "version.h"
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../common/genversion.sh
	DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/../common/genversion.sh CMakeLists.txt
)

target_link_libraries(weather xml2 curl)
