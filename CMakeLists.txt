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

INSTALL(TARGETS tntk
	RUNTIME DESTINATION bin)
