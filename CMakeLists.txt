cmake_minimum_required(VERSION 3.14)
project(Tlang C)

set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra")

find_package(BISON)
find_package(FLEX)

bison_target(
        Parser ${CMAKE_SOURCE_DIR}/src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/Tlang_parser.c
)
flex_target(
        Scanner ${CMAKE_SOURCE_DIR}/src/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/Tlang_scanner.c
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/Tlang_scanner.h
)
add_flex_bison_dependency(Scanner Parser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} include)
add_executable(Tlang
        ${BISON_Parser_OUTPUTS}
        ${FLEX_Scanner_OUTPUTS}
        src/main.c
)