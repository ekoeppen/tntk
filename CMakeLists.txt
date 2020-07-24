cmake_minimum_required (VERSION 2.6) 
project (tntk)
add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

include_directories (/usr/local/include/newt0)
target_link_libraries (tntk newt dl pthread stdc++)

find_library(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)
find_library(READLINE_LIBRARY NAMES readline)
find_library(DCL_LIBRARY NAMES dcl)

target_link_libraries (tntk ${ICONV_LIBRARY})
if (READLINE_LIBRARY)
	add_definitions (-DHAVE_READLINE)
	target_link_libraries (tntk ${READLINE_LIBRARY})
endif (READLINE_LIBRARY)
if (DCL_LIBRARY)
	add_definitions (-DTARGET_OS_POSIX -DHAVE_DCL)
	target_link_libraries (tntk ${DCL_LIBRARY})
endif (DCL_LIBRARY)


INSTALL(TARGETS tntk RUNTIME DESTINATION bin)
