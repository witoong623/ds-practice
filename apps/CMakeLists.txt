add_executable(ds-practice
  main2.cpp
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

find_package(CUDAToolkit REQUIRED)

find_package(NVDS REQUIRED)

target_compile_features(ds-practice PUBLIC cxx_std_20)

target_link_libraries(ds-practice
  PRIVATE
    CUDA::cudart
    PkgConfig::GSTREAMER
    ${NVDS_LIBRARIES}
)

target_include_directories(ds-practice
  PRIVATE
    ${NVDS_INCLUDE_DIRS}
)
