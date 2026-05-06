#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace omc::shared {
    template<typename T>
    class ConcurrentQueue {
    public:
        void push(T value) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(std::move(value));
            }
            cv_.notify_one();
        }

        // Bloqueante
        T wait_and_pop() {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return !queue_.empty() || stopped_;
                });

            if (stopped_ && queue_.empty()) {
                throw std::runtime_error("Queue stopped");
            }

            T value = std::move(queue_.front());
            queue_.pop();
            return value;
        }

        // No bloqueante
        bool try_pop(std::optional<T>& out) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) return false;

            out = std::move(queue_.front());
            queue_.pop();
            return true;
        }

        void stop() {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stopped_ = true;
            }
            cv_.notify_all();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }

    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool stopped_ = false;
    };
}