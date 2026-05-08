#pragma once

#include <memory>

#include "core/event/infraestructure/EventBus.hpp"

namespace omc::server
{
	/// Endpoints
	/// ---------
	/// GET    /health                    – liveness probe
	/// GET    /api/media                 – list all media sources
	/// POST   /api/media                 – add a new media source
	/// GET    /api/media/:id             – get a single media source
	/// DELETE /api/media/:id             – delete a media source
	/// GET    /api/media/:id/show        – show the media source in an overlay
	class ApiServer
	{
	public:
		ApiServer();
		~ApiServer();

		// Habilitamos explícitamente el movimiento (Movable)
		ApiServer(ApiServer&&) noexcept;
		ApiServer& operator=(ApiServer&&) noexcept;

		// Deshabilitamos explícitamente la copia para evitar bugs (Non-copyable)
		ApiServer(const ApiServer&) = delete;
		ApiServer& operator=(const ApiServer&) = delete;

		void start(int port, omc::event::EventBus& eventBus);
		void stop();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};
}