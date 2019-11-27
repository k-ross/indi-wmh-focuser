# - Find wiringPi
# Find the native wiringPi includes and library
# This module defines
#  WIRINGPI_INCLUDE_DIR, where to find wiringPi.h, etc.
#  WIRINGPI_LIBRARIES, the libraries needed to use wiringPi.
#  WIRINGPI_FOUND, If false, do not try to use wiringPi.

find_path(WIRINGPI_INCLUDE_DIR wiringPi.h)
find_library(WIRINGPI_LIBRARY NAMES wiringPi)

# handle the QUIETLY and REQUIRED arguments and set WIRINGPI_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WIRINGPI DEFAULT_MSG WIRINGPI_LIBRARY WIRINGPI_INCLUDE_DIR)

if(WIRINGPI_FOUND)
  set(WIRINGPI_LIBRARIES ${WIRINGPI_LIBRARY})
endif(WIRINGPI_FOUND)

