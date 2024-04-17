set(SOURCES
		main.c
		md.c
		html.c
)

set (CMAKE_C_COMPILER gcc)
set (CMAKE_C_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(mdto ${SOURCES})

install(TARGETS mdto DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
