#pragma once

#include <memory>
#include <stdexcept>

#include "core/event/api/EventBus.hpp"
#include "modules/media/api/IMediaService.hpp"

namespace omc::server
{
	/// REST API server built on top of cpp-httplib.
	///
	/// El servidor arranca en un hilo de fondo (no bloquea el caller de start()).
	/// Llamar a stop() detiene el servidor y une el hilo interno.
	///
	/// Endpoints
	/// ---------
	/// GET    /health                – liveness probe
	///                                 → 200 { "status": "ok" }
	///
	/// GET    /api/media             – lista todas las fuentes de media
	///                                 → 200 [ MediaSourceDto… ]
	///
	/// POST   /api/media             – añade una fuente de media
	///                                 Content-Type: multipart/form-data
	///                                 Campos: media (file, requerido), title (string, opcional)
	///                                 → 201 MediaSourceDto
	///                                 → 400 si falta el fichero o el nombre es inválido
	///
	/// GET    /api/media/:id         – obtiene una fuente de media por ID
	///                                 → 200 MediaSourceDto | 404
	///
	/// DELETE /api/media/:id         – elimina una fuente de media por ID
	///                                 → 204 | 404
	///
	/// GET    /api/media/:id/show    – muestra la fuente de media en el overlay
	///                                 → 200 { "status": "shown" } | 404
	///
	/// Códigos de error comunes
	/// ------------------------
	/// 400  Bad Request   – parámetros inválidos o cuerpo malformado
	/// 404  Not Found     – recurso no encontrado
	/// 500  Internal      – excepción no controlada en el servicio
	class ApiServer
	{
	public:
		ApiServer();
		~ApiServer();

		// Movible (permite almacenarlo en contenedores o transferir propiedad)
		ApiServer(ApiServer&&) noexcept;
		ApiServer& operator=(ApiServer&&) noexcept;

		// No copiable (el servidor posee recursos únicos: socket, hilo)
		ApiServer(const ApiServer&) = delete;
		ApiServer& operator=(const ApiServer&) = delete;

		/// Registra los endpoints y arranca el servidor en un hilo interno.
		/// @param port         Puerto TCP en el que escucha (p. ej. 8080).
		/// @param eventBus     Bus de eventos del dominio.
		/// @param mediaService Servicio de media al que delegan los handlers.
		/// @throws std::logic_error si el servidor ya está en marcha.
		void start(int port,
			omc::event::EventBus& eventBus,
			omc::media::IMediaService& mediaService);

		/// Detiene el servidor y libera todos los recursos internos.
		/// Es seguro llamarlo aunque start() no haya sido invocado.
		void stop();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

} // namespace omc::server