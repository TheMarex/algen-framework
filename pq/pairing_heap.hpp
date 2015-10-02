#pragma once

#include <queue>
#include <utility>

#include "priority_queue.h"

#include "addressable_pairing_heap.hpp"

namespace pq {

template<typename T>
class pairing_heap : public priority_queue<T> {
public:
    pairing_heap() : queue() {}

    static void register_contenders(common::contender_list<priority_queue<T>> &list) {
        using Factory = common::contender_factory<priority_queue<T>>;
        list.register_contender(Factory("pairing_heap", "pairing-heap",
            [](){ return new pairing_heap<T>();}
        ));
    }

    /// Add an element to the priority queue by const lvalue reference
    void push(const T& value) override {
        queue.push(value);
    }
    /// Add an element to the priority queue by rvalue reference (with move)
    void push(T&& value) override {
        queue.push(std::move(value));
    }

    /// Add an element in-place without copying or moving
    template <typename... Args>
    void emplace(Args&&... args) {
        queue.emplace(std::forward<Args>(args)...);
    }

    /// Deletes the top element
    void pop() override {
        queue.pop();
    }

    /// Retrieves the top element
    const T& top() override {
        return queue.top();
    }

    /// Get the number of elements in the priority queue
    size_t size() override {
        return queue.size();
    }

protected:
    addressable_pairing_heap<T> queue;
};

}
