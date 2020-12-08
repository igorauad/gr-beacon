INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_BEACON beacon)

FIND_PATH(
    BEACON_INCLUDE_DIRS
    NAMES beacon/api.h
    HINTS $ENV{BEACON_DIR}/include
        ${PC_BEACON_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    BEACON_LIBRARIES
    NAMES gnuradio-beacon
    HINTS $ENV{BEACON_DIR}/lib
        ${PC_BEACON_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/beaconTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BEACON DEFAULT_MSG BEACON_LIBRARIES BEACON_INCLUDE_DIRS)
MARK_AS_ADVANCED(BEACON_LIBRARIES BEACON_INCLUDE_DIRS)
