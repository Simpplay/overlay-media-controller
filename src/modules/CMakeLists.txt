add_library(modules
	Modules.cpp
)

add_subdirectory(input)
add_subdirectory(media)
add_subdirectory(server)
add_subdirectory(storage)
add_subdirectory(ui)

target_link_libraries(modules
	PRIVATE
		input
		media
		server
		storage
		ui
)