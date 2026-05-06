add_library(modules INTERFACE)

add_subdirectory(input)
add_subdirectory(media)
add_subdirectory(server)
add_subdirectory(storage)
add_subdirectory(ui)

target_link_libraries(modules
	INTERFACE
		input
		media
		server
		storage
		ui
)
