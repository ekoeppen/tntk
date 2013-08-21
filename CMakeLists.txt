cmake_minimum_required (VERSION 2.6) 
project (tntk)
add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

include_directories (/usr/local/include/newt0)
target_link_libraries (tntk cdcl newt dl pthread stdc++)

find_library(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)
find_library(CDCL_LIBRARY NAMES cdcl)
find_library(READLINE_LIBRARY NAMES readline)

target_link_libraries (tntk ${ICONV_LIBRARY})
if (CDCL_LIBRARY)
	add_definitions (-DTARGET_OS_POSIX -DHAS_C99_LONGLONG -DTARGET_RT_BIG_ENDIAN=0 -DTARGET_RT_LITTLE_ENDIAN=1 -DHAVE_CDCL)
	include_directories (/usr/local/include/cDCL)
	target_link_libraries (tntk ${CDCL_LIBRARY})
	message("Enabling NTK connection via cDCL")
endif (CDCL_LIBRARY)
if (READLINE_LIBRARY)
	add_definitions (-DHAVE_READLINE)
	target_link_libraries (tntk ${READLINE_LIBRARY})
endif (READLINE_LIBRARY)


set(CMAKE_OSX_ARCHITECTURES "i386")
set_target_properties(tntk PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
