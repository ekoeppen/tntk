cmake_minimum_required (VERSION 3.6)
project (tntk)
add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

add_subdirectory(NEWT0)

add_definitions (-DTARGET_OS_POSIX)
target_link_libraries (tntk dl pthread stdc++)
target_include_directories(tntk PUBLIC NEWT0/src/newt_core/incs)

find_library(ICONV_LIBRARY NAMES iconv libiconv libiconv-2 c)
find_library(READLINE_LIBRARY NAMES readline)
find_library(DCL_LIBRARY NAMES DCL)

target_link_libraries (tntk ${ICONV_LIBRARY} libnewt)

if (DCL_LIBRARY)
	add_definitions (-DHAVE_DCL)
	target_link_libraries (tntk ${DCL_LIBRARY})
    target_include_directories(tntk PUBLIC /usr/local/include)
endif (DCL_LIBRARY)

if (ICONV_LIBRARY)
	add_definitions (-DHAVE_LIBICONV)
	target_link_libraries (tntk ${ICONV_LIBRARY})
endif (ICONV_LIBRARY)

if (READLINE_LIBRARY)
	add_definitions (-DHAVE_READLINE)
	target_link_libraries (tntk ${READLINE_LIBRARY})
endif (READLINE_LIBRARY)

INSTALL(TARGETS tntk RUNTIME DESTINATION bin)
