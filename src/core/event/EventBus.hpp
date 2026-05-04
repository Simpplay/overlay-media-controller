#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>

#include "Event.hpp"

using EventHandler = std::function<void(const omc::event::Event&)>;

namespace omc::event
{ 
	class EventBus
	{
	public:
		template<typename T>
		void subscribe(std::function<void(const T&)> handler)
		{
			auto wrapper = [handler](const Event& e) {
				handler(static_cast<const T&>(e));
				};

			subscribers[std::type_index(typeid(T))].push_back(wrapper);
		}

		void emit(const omc::event::Event& event);
		void post(std::unique_ptr<omc::event::Event>);

		void processQueue();

	private:
		std::unordered_map<std::type_index, std::vector<EventHandler>> subscribers{};

		std::queue<std::unique_ptr<Event>> queue{};
		std::mutex mtx;

		void dispatch(const omc::event::Event& event);
	};
}