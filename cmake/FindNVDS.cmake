# from https://forums.developer.nvidia.com/t/deepstream-pkg-config-file/120889/13

# Select install dir using provided version.
SET(NVDS_INSTALL_DIR /opt/nvidia/deepstream/deepstream-${NVDS_FIND_VERSION})

# List all libraries in deepstream.
SET(NVDS_LIBS
  nvds_meta
  nvds_yml_parser

  nvbufsurface

  nvdsgst_meta
  nvdsgst_helper
)

message(STATUS "NVDS_INSTALL_DIR ${NVDS_INSTALL_DIR}")

# Find all libraries in list.
foreach(LIB ${NVDS_LIBS})
  find_library(${LIB}_PATH NAMES ${LIB} PATHS ${NVDS_INSTALL_DIR}/lib)

  if(${LIB}_PATH)
    set(NVDS_LIBRARIES ${NVDS_LIBRARIES} ${${LIB}_PATH})
  else()
    message(FATAL ERROR " Unable to find lib: ${LIB}")
    set(NVDS_LIBRARIES FALSE)
    break()
  endif()
endforeach()

# Find include directories.
find_path(NVDS_INCLUDE_DIRS
  NAMES
    nvds_version.h
  HINTS
    ${NVDS_INSTALL_DIR}/sources/includes
)

# Check libraries and includes.
if (NVDS_LIBRARIES AND NVDS_INCLUDE_DIRS)
  set(NVDS_FOUND TRUE)
else()
  message(FATAL ERROR " Unable to find NVDS")
endif()
