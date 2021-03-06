#pragma once

#include "../common/maybe.h"

using namespace common::monad;

namespace hashtable {

template <typename Key, typename T>
class hashtable {
public:
    using value_type = std::pair<Key, T>;
    using key_type = Key;
    using mapped_type = T;

    // You also need to provide the following:
    // static void register_contenders(common::contender_list<hashtable<Key, T>> &list)

    /// Retrieve a key's value, or insert a default-constructed value if not found
    virtual T& operator[](const Key &key) = 0;

    /// Retrieve a key's value by rvalue reference, or insert a default-constructed value if not found
    virtual T& operator[](Key &&key) = 0;

    /// Find a key in the hash table
    virtual maybe<T> find(const Key &key) const = 0;

    /// Erases all elements with the given key
    /// Returns the number of elements removed
    virtual size_t erase(const Key &key) = 0;

    /// Returns the number of elements
    virtual size_t size() const = 0;

    /// Clear the hash table
    virtual void clear() = 0;

    /// Virtual destructor to allow destruction through derived pointer
    virtual ~hashtable() {}
};
}
