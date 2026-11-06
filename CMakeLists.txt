
cmake_minimum_required(VERSION 3.28)

project(disque-obs VERSION 0.7.0 LANGUAGES C)

find_package(libobs REQUIRED)

add_library(disque-obs MODULE
    disque.c
)

target_link_libraries(disque-obs PRIVATE OBS::libobs)

set_target_properties(disque-obs PROPERTIES
    PREFIX ""
    OUTPUT_NAME "disque-obs"
)
