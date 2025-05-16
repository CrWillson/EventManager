#pragma once

#include <atomic>
#include <concepts>

class TypeIdentification {
public:
    using TypeID = size_t;

    template <typename Event_T>
    static inline TypeID getID() {
        static TypeID id = getNextID();
        return id;
    }

private:

    static inline TypeID getNextID() {
        static std::atomic<TypeID> type_counter{0};
        return type_counter.fetch_add(1, std::memory_order_relaxed);
    }
};

namespace Event {
    struct EventBase {
        virtual ~EventBase() = default;
    };

    template<typename T>
    concept is_event_type = std::derived_from<T, EventBase>;

    struct IntEvent : EventBase {
        IntEvent(int d) : data(d) {}
        int data;
    };

    template<typename T>
    struct TemplateEvent : EventBase {
        TemplateEvent(T d) : data(d) {}
        T data;
    };
    
}

