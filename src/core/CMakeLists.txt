add_library(core INTERFACE)

add_subdirectory(event)
add_subdirectory(threading)
add_subdirectory(types)
add_subdirectory(updater)

target_link_libraries(core
	INTERFACE
		event
		threading
		types
		updater
)