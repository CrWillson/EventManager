#pragma once

#include "event_types.hpp"

#include <functional>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>

class EventManager {
private:
    // Private constructor to prevent instantiation
    EventManager();

    // Private method to get a unique ID to assign to every handler function
    using HandlerID = size_t;
    static inline HandlerID getUniqueHandlerID() {
        static std::atomic<HandlerID> handler_counter{0};
        return handler_counter.fetch_add(1, std::memory_order_relaxed);
    }

    // Type aliases for handler functions
    using HandlerFunc = std::function<void(const Event::EventBase&)>;
    using HandlerPair = std::pair<HandlerID, HandlerFunc>;

    // Maps event type to all the handlers subscribed to that event type
    std::map<TypeIdentification::TypeID, std::vector<HandlerPair>> handlers;

    // Subscription object returned from subscribe(). Used to unsubscribe that handler later.
    class subscription_t {
    public:
        ~subscription_t() = default;
        subscription_t(const subscription_t&) = delete;
        subscription_t& operator=(const subscription_t&) = delete;
    private:
        subscription_t(HandlerID handlerID, TypeIdentification::TypeID eventID) 
            : handler_id(handlerID), event_id(eventID) {}

        HandlerID handler_id;
        TypeIdentification::TypeID event_id;

        friend class EventManager;
    };

    // Incoming queue of events to process
    std::queue<std::function<void()>> eventQueue;

    // Event queue thread protection
    mutable std::mutex eventQueueMutex;
    std::condition_variable eventCond;
    std::future<void> eventLoopFuture;

    // Flag to halt the event processing loop
    std::atomic<bool> runEventLoop{false};

    // Event processing loop meant to be run in a detached thread
    void eventLoop();



public:
    using Subscription = std::unique_ptr<subscription_t>;

    // Halt the event processing loop on destruction
    ~EventManager();

    // Make the event manager a singleton class
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    static EventManager& getInstance() {
        static EventManager instance;
        return instance;
    }

    // Start the event processing loop in another thread
    bool start();
    
    // Halt the event processing loop thread
    bool halt();

    // Remove a handler from the event manager. Invalidates the Subscription
    bool unsubscribe(Subscription handlerSub);

    
    // Subscribe a handler function to a specific event type
    template <Event::is_event_type EventType>
    Subscription subscribe(std::function<void(const EventType&)> handler) {
        if (!handler) {
            throw std::invalid_argument("EventManager: Handler cannot be null");
        }

        auto typeID = TypeIdentification::getID<EventType>();
        
        HandlerFunc wrappedHandler = [handler](const Event::EventBase& event) {
            const auto* derived = static_cast<const EventType*>(&event);
            handler(*derived);
        };
        auto handlerID = getUniqueHandlerID();

        handlers[typeID].emplace_back(handlerID, wrappedHandler);
                
        return Subscription(new subscription_t(handlerID, typeID));
    }

    // Publish an event to the event queue
    template <Event::is_event_type EventType>
    void publish(const EventType& event) {
        std::size_t typeID = TypeIdentification::getID<EventType>();

        auto it = handlers.find(typeID); 
        if (it == handlers.end()) {
            return;
        }

        std::vector<HandlerFunc> handlersToRun;
        for (auto& [handlerID, handlerFunc] : it->second) {
            handlersToRun.push_back(handlerFunc);
        }

        auto action = [event = event, handlersToRun = std::move(handlersToRun)]{
            for (auto& handler : handlersToRun) {
                handler(event);
            }
        };

        {
            std::lock_guard<std::mutex> lock(eventQueueMutex);
            eventQueue.push(std::move(action));
        }
        eventCond.notify_one();

    }
};