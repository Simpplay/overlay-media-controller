add_executable(app main.cpp)

# Se añaden fuentes al target de forma estructurada
target_sources(app PRIVATE
    "runtime/OverlayMediaController.hpp"
    "runtime/OverlayMediaController.cpp"
    "api/Event.cpp"
)

# Aquí sí van las LIBRERÍAS de las que depende la aplicación
target_link_libraries(app PRIVATE omc)