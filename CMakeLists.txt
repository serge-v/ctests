cmake_minimum_required(VERSION 2.6)

add_executable(curltest	curltest.c)
target_link_libraries(curltest curl)

add_executable(rextest rextest.c)
target_link_libraries(rextest)

add_executable(departures departures.c fetch.c)
target_link_libraries(departures curl)

