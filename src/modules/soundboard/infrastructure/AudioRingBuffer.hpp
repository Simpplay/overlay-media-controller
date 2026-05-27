#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <algorithm>

namespace omc::soundboard {

    /**
     * @brief A thread-safe circular buffer for PCM audio data.
     * 
     * Modified for a single-producer, multiple-consumer scenario.
     * Each consumer has its own read pointer.
     */
    class AudioRingBuffer {
    public:
        explicit AudioRingBuffer(size_t capacity_bytes)
            : buffer(capacity_bytes), head(0), done(false) {
            // Initialize reader offsets for 2 consumers
            reader_offsets.assign(2, 0);
            reader_counts.assign(2, 0);
        }

        /**
         * @brief Writes data to the buffer. Blocks if any consumer is too far behind.
         */
        void write(const void* data, size_t size) {
            const std::byte* src = static_cast<const std::byte*>(data);
            size_t remaining = size;

            while (remaining > 0) {
                std::unique_lock<std::mutex> lock(mutex);
                
                // Wait until there's space for ALL consumers
                cv_write.wait(lock, [this]() { 
                    bool any_full = false;
                    for (size_t count : reader_counts) {
                        if (count >= buffer.size()) {
                            any_full = true;
                            break;
                        }
                    }
                    return !any_full;
                });

                // Calculate how much we can write based on the "slowest" consumer
                size_t max_count = 0;
                for (size_t count : reader_counts) {
                    max_count = std::max(max_count, count);
                }
                
                size_t available_space = buffer.size() - max_count;
                size_t to_write = std::min(remaining, available_space);
                
                size_t first_part = std::min(to_write, buffer.size() - head);
                std::copy(src, src + first_part, buffer.begin() + head);
                
                if (to_write > first_part) {
                    std::copy(src + first_part, src + to_write, buffer.begin());
                }

                head = (head + to_write) % buffer.size();
                for (size_t& count : reader_counts) {
                    count += to_write;
                }
                
                src += to_write;
                remaining -= to_write;

                lock.unlock();
                cv_read.notify_all();
            }
        }

        /**
         * @brief Reads data from the buffer for a specific consumer.
         */
        size_t read(size_t reader_id, void* data, size_t size) {
            if (reader_id >= reader_offsets.size()) return 0;

            std::byte* dst = static_cast<std::byte*>(data);
            std::unique_lock<std::mutex> lock(mutex);

            size_t count = reader_counts[reader_id];
            if (count == 0) return 0;

            size_t to_read = std::min(size, count);
            size_t tail = reader_offsets[reader_id];
            
            size_t first_part = std::min(to_read, buffer.size() - tail);
            std::copy(buffer.begin() + tail, buffer.begin() + tail + first_part, dst);
            
            if (to_read > first_part) {
                std::copy(buffer.begin(), buffer.begin() + (to_read - first_part), dst + first_part);
            }

            reader_offsets[reader_id] = (tail + to_read) % buffer.size();
            reader_counts[reader_id] -= to_read;

            lock.unlock();
            cv_write.notify_all();
            return to_read;
        }

        void setDone(bool is_done = true) {
            std::lock_guard<std::mutex> lock(mutex);
            done = is_done;
            cv_read.notify_all();
        }

        bool isDone(size_t reader_id) const {
            std::lock_guard<std::mutex> lock(mutex);
            if (reader_id >= reader_counts.size()) return done;
            return done && reader_counts[reader_id] == 0;
        }

        void reset() {
            std::lock_guard<std::mutex> lock(mutex);
            head = 0;
            std::fill(reader_offsets.begin(), reader_offsets.end(), 0);
            std::fill(reader_counts.begin(), reader_counts.end(), 0);
            done = false;
        }

    private:
        std::vector<std::byte> buffer;
        size_t head;
        std::vector<size_t> reader_offsets;
        std::vector<size_t> reader_counts;
        bool done;
        mutable std::mutex mutex;
        std::condition_variable cv_read;
        std::condition_variable cv_write;
    };

} // namespace omc::soundboard
