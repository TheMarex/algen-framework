#pragma once

#include <queue>
#include <utility>

#include "priority_queue.h"

#include "addressable_pairing_heap.hpp"
#include "addressable_pairing_heap_vector.hpp"
#include "helper/free_list.hpp"

namespace pq {

template<typename T,
         template<typename T0, typename C, template<typename S0> class F> class PairingHeapT=addressable_pairing_heap,
         template<typename S> class FreeListT=malloc_wrapper>
class pairing_heap : public priority_queue<T> {
public:
    pairing_heap() : queue() {}

    static void register_contenders(common::contender_list<priority_queue<T>> &list) {
        using Factory = common::contender_factory<priority_queue<T>>;

        list.register_contender(Factory("pairing_heap vector", "pairing-heap-vector",
            [](){ return new pairing_heap<T, addressable_pairing_heap_vector>();}
        ));
        list.register_contender(Factory("pairing_heap linked", "vector-pairing-linked",
            [](){ return new pairing_heap<T, addressable_pairing_heap>();}
        ));
        //list.register_contender(Factory("pairing_heap with free list", "pairing-heap-fl",
        //    [](){ return new pairing_heap<T, free_list>();}
        //));
        //list.register_contender(Factory("pairing_heap lazy-shrink free list", "pairing-heap-lazyshrink-fl",
        //    [](){ return new pairing_heap<T, ls_free_list>();}
        //));
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
    PairingHeapT<T, std::less<T>, FreeListT> queue;
};

}
