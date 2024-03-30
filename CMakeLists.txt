cmake_minimum_required(VERSION 3.22)

project(ds-practice LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

SET(NVDS_FIND_VERSION 6.4)

add_subdirectory(apps)
