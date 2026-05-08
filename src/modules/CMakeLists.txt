add_library(modules INTERFACE)

add_subdirectory(media)
add_subdirectory(server)
add_subdirectory(ui)

target_link_libraries(modules
	INTERFACE
		media
		server
		ui
)
