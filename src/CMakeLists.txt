set(SOURCES
		main.c
		params.c
		md.c
		html.c
		syntax.c
		xml.c
		footnotes.c
		uuid.c
)

set (CMAKE_C_COMPILER gcc)
set (CMAKE_C_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_compile_options(-Wall -Wextra)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(md2html ${SOURCES})

install(TARGETS md2html DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
