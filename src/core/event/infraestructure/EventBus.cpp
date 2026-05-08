#include "EventBus.hpp"

namespace omc::event
{
	void EventBus::emit(const omc::event::Event& event)
	{
		auto it = subscribers.find(typeid(event));
		if (it != subscribers.end()) {
			for (const auto& handler : it->second) {
				handler(event);
			}
		}
	}

	void EventBus::post(std::unique_ptr<omc::event::Event> event)
	{
		std::lock_guard<std::mutex> lock(mtx);
		queue.push(std::move(event));
	}

	void EventBus::processQueue() {
		std::queue<std::unique_ptr<Event>> localQueue;

		{
			std::lock_guard<std::mutex> lock(mtx);
			std::swap(localQueue, queue);
		}

		while (!localQueue.empty()) {
			auto& event = localQueue.front();
			dispatch(*event);
			localQueue.pop();
		}
	}

	void EventBus::dispatch(const omc::event::Event& event)
	{
		auto it = subscribers.find(typeid(event));
		if (it != subscribers.end()) {
			for (const auto& handler : it->second) {
				handler(event);
			}
		}
	}
}