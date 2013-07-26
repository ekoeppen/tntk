cmake_minimum_required (VERSION 2.6) 
project (tntk)
add_definitions (-DTARGET_OS_POSIX -DHAS_C99_LONGLONG -DTARGET_RT_BIG_ENDIAN=1 -DTARGET_RT_LITTLE_ENDIAN=0)
include_directories (/usr/local/include/cDCL /usr/local/include/newt0)


add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

target_link_libraries (tntk cdcl newt dl pthread stdc++)

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
