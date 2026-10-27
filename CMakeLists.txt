# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# idf_component_register(SRCS "main.c"
#                     INCLUDE_DIRS "."
#                     PRIV_REQUIRES shtc3)
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "../../P3/components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(project)
