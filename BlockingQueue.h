#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

#include "noncopyable.h"

namespace buzz
{
    template<typename T>
    class BlockingQueue : noncopyable
    {
    public:
        explicit BlockingQueue(const size_t capacity = 0)
            : m_capacity(capacity)
        { }

        bool Put(const T& elem, const int timeout = -1)
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_capacity && m_queue.size() == m_capacity) {
                if (timeout == 0) {
                    return false;
                }

                if (timeout == -1) {
                    m_not_full.wait(lock, [this] { return m_queue.size() != m_capacity; });
                }
                else {
                    auto t = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

                    while (m_not_empty.wait_until(lock, t) != std::cv_status::timeout &&
                        m_queue.size() != m_capacity)
                    { }

                    if (m_queue.size() == m_capacity) {
                        return false;
                    }
                }
            }

            m_queue.push_back(elem);
            m_not_empty.notify_one();

            return true;
        }

        bool Poll(T& elem, const int timeout = -1)
        {
            std::unique_lock<std::mutex> lock(m_mutex);

            if (m_queue.empty()) {
                if (timeout == 0) {
                    return false;
                }

                if (timeout == -1) {
                    m_not_empty.wait(lock, [this] { return !m_queue.empty(); });
                }
                else {
                    auto t = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);

                    while (m_not_empty.wait_until(lock, t) != std::cv_status::timeout &&
                        m_queue.empty())
                    { }

                    if (m_queue.empty()) {
                        return false;
                    }
                }
            }

            elem = std::move(m_queue.front());
            m_queue.pop_front();

            m_not_full.notify_one();

            return true;
        }

        bool Empty()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

    private:
        std::deque<T> m_queue;
        const size_t m_capacity;

        std::mutex m_mutex;
        std::condition_variable m_not_full;
        std::condition_variable m_not_empty;
    };
}