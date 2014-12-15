cmake_minimum_required(VERSION 2.6)

add_executable(curltest	curltest.c)
target_link_libraries(curltest curl)

add_executable(rextest rextest.c fetch.c)
target_link_libraries(rextest curl)

