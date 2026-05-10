#pragma once

#include <vector>
#include <map>

#include "modules/ui/domain/UiWindow.hpp"

namespace omc::ui 
{
	class UiRepository
	{
	public:
		std::map<int, std::unique_ptr<window::UiWindow>>& getWindows() { return windows; }

		int getWindowCount() const { return static_cast<int>(windows.size()); }

		void addWindow(std::unique_ptr<window::UiWindow> window) { windows[getNextWindowId()] = std::move(window); }
		void removeWindow(int id) { windows.erase(id); }

		void updateWindows() {
			window::UiWindow* highestZWindow = nullptr;
			int highestZ = -1;
			for (const auto& [id, window] : windows) {
				window->update();

				if (window->canInteractWithWindow()) {
					if (window->getZBase() > highestZ) {
						highestZ = window->getZBase();
						highestZWindow = window.get();
					}
				}

				if (window->isInteractingWithTitleBar()) {
					highestZWindow = window.get();
					highestZ = LONG_MAX; // Asegura que esta ventana quede al frente
				}
			}

			if (highestZWindow) {
				highestZWindow->updateWindowInteraction();
			}

			windows.erase(std::remove_if(windows.begin(), windows.end(), [](const auto& pair) {
				return pair.second->closed();
				}), windows.end());
		}

		void renderWindows(std::vector<DrawCommand>& out) {
			for (const auto& [id, window] : windows) {
				window->buildDrawCommand(out);
			}
		}

		int getNextWindowId() const {
			int maxId = 0;
			for (const auto& [id, _] : windows) {
				if (id > maxId) {
					maxId = id;
				}
			}
			return maxId + 1;
		}

	private:
		std::map<int, std::unique_ptr<window::UiWindow>> windows;
	};
}