cmake_minimum_required (VERSION 2.6) 
project (tntk)
find_library(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)
find_library(CDCL_LIBRARY NAMES cdcl)

if (${CDCL_LIBRARY})
set (CDCL_FLAG -DHAVE_CDCL)
endif (${CDCL_LIBRARY})

add_definitions (-DTARGET_OS_POSIX -DHAS_C99_LONGLONG -DTARGET_RT_BIG_ENDIAN=1 -DTARGET_RT_LITTLE_ENDIAN=0 ${CDCL_FLAG})
include_directories (/usr/local/include/cDCL /usr/local/include/newt0)

add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

set(CMAKE_OSX_ARCHITECTURES "i386")
set_target_properties(tntk PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
target_link_libraries (tntk cdcl newt dl pthread stdc++ ${ICONV_LIBRARY} ${CDCL_LIBRARY})

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
