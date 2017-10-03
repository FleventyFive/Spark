file(GLOB_RECURSE sources
	*.cpp
)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(
	${PROJECT_NAME}
	${CURSES_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_SOURCE_DIR})