add_executable(person-monitor
  deepstream_test2_app.c
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

find_package(CUDAToolkit REQUIRED)

find_package(NVDS REQUIRED)

# find_package(X11 REQUIRED)

target_link_libraries(person-monitor
  PRIVATE
    CUDA::cudart
    PkgConfig::GSTREAMER
    ${NVDS_LIBRARIES}
)

target_include_directories(person-monitor
  PRIVATE
    ${NVDS_INCLUDE_DIRS}
)
