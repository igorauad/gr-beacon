find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_BEACON gnuradio-beacon)

FIND_PATH(
    GR_BEACON_INCLUDE_DIRS
    NAMES gnuradio/beacon/api.h
    HINTS $ENV{BEACON_DIR}/include
        ${PC_BEACON_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_BEACON_LIBRARIES
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

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-beaconTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_BEACON DEFAULT_MSG GR_BEACON_LIBRARIES GR_BEACON_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_BEACON_LIBRARIES GR_BEACON_INCLUDE_DIRS)
