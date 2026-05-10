cmake_minimum_required (VERSION 3.31)
project (tntk)
option(USE_DCL "Use DCL when found" ON)
add_executable (tntk
package.cpp
part.cpp
preferences.cpp
tntk.cpp
)

if (WIN32)
    add_definitions(-D__WIN32__)
    target_sources(tntk PRIVATE win32/getopt.c)
    target_include_directories(tntk PRIVATE win32)
    target_compile_definitions(tntk PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  newt0
  GIT_REPOSITORY https://github.com/ekoeppen/NEWT0.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(newt0)

add_definitions (-DTARGET_OS_POSIX)
if (UNIX)
    target_link_libraries (tntk dl pthread stdc++)
endif()
target_include_directories(tntk PUBLIC ${newt0_SOURCE_DIR}/src/newt_core/incs)

find_library(READLINE_LIBRARY NAMES readline)
if (USE_DCL)
	find_library(DCL_LIBRARY NAMES DCL)
endif (USE_DCL)

target_link_libraries (tntk libnewt)

if (DCL_LIBRARY)
	add_definitions (-DHAVE_DCL)
	target_link_libraries (tntk ${DCL_LIBRARY})
    target_include_directories(tntk PUBLIC /usr/local/include)
endif (DCL_LIBRARY)

if (READLINE_LIBRARY)
	add_definitions (-DHAVE_READLINE)
	target_link_libraries (tntk ${READLINE_LIBRARY})
endif (READLINE_LIBRARY)

INSTALL(TARGETS tntk RUNTIME DESTINATION bin)
