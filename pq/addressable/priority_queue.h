#pragma once

namespace pq {

namespace addressable {

template <typename T>
class priority_queue {
public:
    using value_type = T;
    using handle_type = void*;

    // You also need to provide the following:
    // static void register_contenders(common::contender_list<priority_queue<T>> &list)

    /// Add an element to the priority queue by const lvalue reference
    virtual handle_type push(const T& value) = 0;
    /// Add an element to the priority queue by rvalue reference (with move)
    virtual handle_type push(T&& value) = 0;

    /// Modifies the key of the given element so that it moves towards the top
    /// Only supports updates with Compare(old_key, new_key)
    virtual void modify_up(handle_type handle, const T& value) = 0;

    /// Modifies the key of the given element
    virtual void modify(handle_type handle, const T& value) = 0;

    /// Deletes the top element
    virtual void pop() = 0;

    /// Retrieves the top element
    virtual const T& top() = 0;

    /// Get the number of elements in the priority queue
    virtual size_t size() = 0;

    /// Virtual destructor needed for inheritance
    virtual ~priority_queue() {}
};

}

}
