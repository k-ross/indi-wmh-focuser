cmake_minimum_required(VERSION 3.10)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT(indi-wmh-focuser VERSION 1.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS}")

set(PROGRAM_PERMISSIONS_DEFAULT
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

find_package(INDI REQUIRED)
find_package(PkgConfig REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})

configure_file(config.h.in config.h)
configure_file(indi_wmh_focuser.xml.in indi_wmh_focuser.xml)

if(ROCKPI)
    pkg_search_module(MRAA REQUIRED mraa)
    set(indi_wmh_focuser_SRCS
        wmh_focuser.cpp
        mraamotor.cpp
    )
    set(EXTRA_LIBS ${MRAA_LIBRARIES})
else()
    pkg_search_module(GPIOD REQUIRED libgpiod)
    pkg_search_module(GPIODCXX REQUIRED libgpiodcxx)
    set(indi_wmh_focuser_SRCS
        wmh_focuser.cpp
        gpiomotor.cpp
    )
    set(EXTRA_LIBS ${GPIODCXX_LIBRARIES})
endif()

add_executable(indi_wmh_focuser ${indi_wmh_focuser_SRCS})
target_link_libraries(indi_wmh_focuser ${INDI_LIBRARIES} pthread ${EXTRA_LIBS})
install(TARGETS indi_wmh_focuser RUNTIME DESTINATION bin
    PERMISSIONS ${PROGRAM_PERMISSIONS_DEFAULT} SETUID)
install(FILES "${PROJECT_BINARY_DIR}/indi_wmh_focuser.xml" DESTINATION ${INDI_DATA_DIR})
