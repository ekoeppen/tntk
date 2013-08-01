cmake_minimum_required (VERSION 2.6) 
project (tntk)
include_directories (/usr/local/include/newt0)

add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

set(CMAKE_OSX_ARCHITECTURES "i386")
set_target_properties(tntk PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
target_link_libraries (tntk newt dl stdc++)

FIND_PATH(ICONV_INCLUDE_DIR iconv.h)
FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
   set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
   set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
