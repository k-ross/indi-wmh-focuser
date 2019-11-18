cmake_minimum_required(VERSION 2.4.7)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

PROJECT(indi-wmh-focuser CXX C)
include(ExternalProject)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
set(MAKE make)

find_package(INDI REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})

set(indi_wmh_focuser_SRCS
        wmh_focuser.cpp
	DEV_Config.c
	DRV8825.c
   )

add_executable(indi_wmh_focuser ${indi_wmh_focuser_SRCS})
target_link_libraries(indi_wmh_focuser indidriver wiringPi)
install(TARGETS indi_wmh_focuser RUNTIME DESTINATION bin)
install(FILES indi_wmh_focuser.xml DESTINATION ${INDI_DATA_DIR})