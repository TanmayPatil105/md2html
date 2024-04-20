set(SOURCES
		main.c
		md.c
		html.c
)

set (CMAKE_C_COMPILER gcc)
set (CMAKE_C_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(md2html ${SOURCES})

install(TARGETS md2html DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
