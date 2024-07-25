add_executable(ds-practice
  Analytic.cpp
  BufferLedger.cpp
  FrameBuffer.cpp
  Geometry.cpp
  main.cpp
  MovementAnalyzer.cpp
  Pipeline.cpp
  PipelineCallback.cpp
  ThreadPool.cpp
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

find_package(CUDAToolkit REQUIRED)

find_package(OpenCV REQUIRED)

find_package(NVDS REQUIRED)

target_compile_features(ds-practice PUBLIC cxx_std_20)

target_link_libraries(ds-practice
  PRIVATE
    CUDA::cudart
    PkgConfig::GSTREAMER
    ${OpenCV_LIBS}
    ${NVDS_LIBRARIES}
)

target_include_directories(ds-practice
  PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${NVDS_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
