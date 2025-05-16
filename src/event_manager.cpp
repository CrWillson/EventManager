#include "event_manager.hpp"

#include <algorithm>

EventManager::EventManager()
{
    start();
}

EventManager::~EventManager() 
{
    halt();
}

void EventManager::eventLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(eventQueueMutex);
            eventCond.wait(lock, [this] {
                return !eventQueue.empty() || !runEventLoop.load(std::memory_order_acquire);
            });

            if (!runEventLoop.load(std::memory_order_acquire) && eventQueue.empty()) {
                break; // Stop requested and no events left
            }

            task = std::move(eventQueue.front());
            eventQueue.pop();
        }

        task();
    }
}


bool EventManager::start()
{
    bool expected = false;
    bool desired = true;
    if (!runEventLoop.compare_exchange_strong(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed)) {
        return false;
    }

    eventLoopFuture = std::async(std::launch::async, [this]{ this->eventLoop(); });
    return true;
}

bool EventManager::halt()
{
    bool expected = true;
    bool desired = false;
    if (!runEventLoop.compare_exchange_strong(expected, desired, std::memory_order_relaxed, std::memory_order_relaxed)) {
        return false;
    }

    eventCond.notify_all();

    if (eventLoopFuture.valid()) {
        eventLoopFuture.get();
    }

    return true;
}

bool EventManager::unsubscribe(Subscription handlerSub) 
{
    if (!handlerSub) {
        return false;
    }

    auto it = handlers.find(handlerSub->event_id);
    if (it == handlers.end()) {
        return false;
    }

    auto& vec = it->second;
    auto pos = std::remove_if(vec.begin(), vec.end(), [&](const auto& pair) {
        return pair.first == handlerSub->handler_id;
    });

    if (pos != vec.end()) {
        vec.erase(pos, vec.end());
        if (vec.empty()) {
            handlers.erase(it);
        }
        return true;
    }

    return false;
}

