#pragma once

// This class uses features from GNU libstdc++'s policy-base
// data structures library
#if defined(__GNUG__) && !(defined(__APPLE_CC__))

#include <ext/pb_ds/priority_queue.hpp>
#include <utility>

#include "priority_queue.h"

namespace pq {

namespace addressable {

template<typename T,
         typename Cmp_Fn = std::less<T>,
         typename Tag = __gnu_pbds::pairing_heap_tag,
         typename Allocator = std::allocator<char>>
class gnu_pq : public priority_queue<T> {
public:
    using queue_type = __gnu_pbds::priority_queue<T, Cmp_Fn, Tag, Allocator>;
    using internal_handle = decltype(typename queue_type::point_iterator().m_p_nd);
    gnu_pq() : queue() {}

    virtual ~gnu_pq() {
        bool is_pairing_heap = std::is_same<Tag,  __gnu_pbds::pairing_heap_tag>::value;
        if (is_pairing_heap) {
            // Pairing heap has a recursive destructor
            queue.clear();
        }
    };

    static void register_contenders(common::contender_list<priority_queue<T>> &list) {
        using Factory = common::contender_factory<priority_queue<T>>;
        // The pairing heap has a recursive destructor, be careful
        list.register_contender(Factory("GNU Pairing Heap", "GNU-pairing-heap",
            [](){ return new gnu_pq<T, std::less<T>, __gnu_pbds::pairing_heap_tag>();}
        ));
    }

    /// Add an element to the priority queue by const lvalue reference
    void* push(const T& value) override {
        return static_cast<void*>(queue.push(value).m_p_nd);
    }
    /// Add an element to the priority queue by rvalue reference (with move)
    void* push(T&& value) override {
        return static_cast<void*>(queue.push(std::move(value)).m_p_nd);
    }

    /// Modify the key of an element
    void modify(void* handle, const T& value) override {
        queue.modify(typename queue_type::point_iterator {static_cast<internal_handle>(handle)}, value);
    }

    /// Modify the key of an element
    /// FIXME GNU PQ does not offer specialized handling for this case
    void modify_up(void* handle, const T& value) override {
        modify(handle, value);
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
    queue_type queue;
};

}

#endif
