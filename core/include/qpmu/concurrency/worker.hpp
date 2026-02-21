#pragma once

#include <atomic>
#include <thread>

struct Worker
{
public:
    Worker() = default;

    Worker(const Worker &) = delete;
    Worker &operator=(const Worker &) = delete;
    Worker(Worker &&) = delete;
    Worker &operator=(Worker &&) = delete;

    template <class F, class... Args>
    void start(F &&f, Args &&...args)
    {
        // Prevent accidental double-start
        if (_thread.joinable())
            throw std::logic_error("Worker already started");

        _thread = std::thread([this, func = std::forward<F>(f),
                               args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            _running.store(true, std::memory_order_relaxed);

            try {
                std::apply(func, args);
            } catch (...) {
                _running.store(false, std::memory_order_relaxed);
                throw;
            }

            _running.store(false, std::memory_order_relaxed);
        });
    }

    bool running() const noexcept { return _running.load(std::memory_order_relaxed); }
    void join_thread()
    {
        if (_thread.joinable())
            _thread.join();
    }

    ~Worker()
    {
        if (_thread.joinable())
            _thread.join();
    }

private:
    std::thread _thread = {};
    std::atomic<bool> _running = false;
};
