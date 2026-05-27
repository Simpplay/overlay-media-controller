add_library(modules INTERFACE)

add_subdirectory(media)
add_subdirectory(server)
add_subdirectory(ui)
add_subdirectory(soundboard)

target_link_libraries(modules
	INTERFACE
		media
		server
		ui
		soundboard
)
