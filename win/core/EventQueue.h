#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>


namespace core {

template <typename T>
class EventQueue {
public:
    explicit EventQueue(std::size_t capacity = 1000) : capacity(capacity) {}

    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            items.push_back(std::move(item));
            while (items.size() > capacity) {
                items.pop_front();
            }
        }
        condition.notify_one();
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        if (items.empty()) {
            return false;
        }
        item = std::move(items.front());
        items.pop_front();
        return true;
    }

    bool waitAndPop(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!condition.wait_for(lock, timeout, [this] { return !items.empty(); })) {
            return false;
        }
        item = std::move(items.front());
        items.pop_front();
        return true;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return items.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        items.clear();
    }

private:
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::size_t capacity;
    std::deque<T> items;
};

}
