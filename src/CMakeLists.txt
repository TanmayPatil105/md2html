set(SOURCES
		main.c
		params.c
		md.c
		html.c
)

set (CMAKE_C_COMPILER gcc)
set (CMAKE_C_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(md2html ${SOURCES})

install(TARGETS md2html DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
