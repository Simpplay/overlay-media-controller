add_library(core INTERFACE)

add_subdirectory(event)
add_subdirectory(threading)
add_subdirectory(types)
add_subdirectory(updates)

target_link_libraries(core
	INTERFACE
		event
		threading
		types
		updates
)