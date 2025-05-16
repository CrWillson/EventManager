#include "event_manager.hpp"
#include <fmt/core.h>

#include <gtest/gtest.h>


TEST(EventManager, test_usage)
{
    struct A {
        int value;
    };

    auto& man = EventManager::getInstance();

    auto intReg1 = man.subscribe<Event::IntEvent>([&](const auto& e) {
        fmt::println("1. IntEvent detected. Data: {}", e.data);
    });
    man.subscribe<Event::IntEvent>([&](const auto& e) {
        fmt::println("2. IntEvent detected. Data: {}", e.data);
    });
    man.subscribe<Event::IntEvent>([&](const auto& e) {
        fmt::println("3. IntEvent detected. Data: {}", e.data);
    });
    auto temRegD = man.subscribe<Event::TemplateEvent<double>>([&](const auto& e) {
        fmt::println("1. TemplateEvent<double> detected. Data: {}", e.data);
    });
    man.subscribe<Event::TemplateEvent<A>>([&](const auto& e) {
        fmt::println("1. TemplateEvent<A> detected. Data: {}", e.data.value);
    });
    auto temRegA2 = man.subscribe<Event::TemplateEvent<A>>([&](const auto& e) {
        fmt::println("2. TemplateEvent<A> detected. Data: {}", e.data.value);
    });

    Event::TemplateEvent<A> tempEvent{A{10}};
    Event::IntEvent intEvent{5};
    Event::TemplateEvent<double> doubleEvent{4.5};

    man.publish(intEvent);
    man.publish(tempEvent);
    man.publish(doubleEvent);

    man.unsubscribe(std::move(intReg1));
    man.unsubscribe(std::move(temRegA2));
    man.unsubscribe(std::move(temRegD));

    man.publish(intEvent);
    man.publish(tempEvent);
    man.publish(doubleEvent);


    man.halt();
}