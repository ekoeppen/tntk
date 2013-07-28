cmake_minimum_required (VERSION 2.6) 
project (tntk)
include_directories (/usr/local/include/newt0)
set(CMAKE_OSX_ARCHITECTURES "i386")

add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

target_link_libraries (tntk newt dl stdc++ iconv)

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
