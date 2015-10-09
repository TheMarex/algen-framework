#pragma once

#include <deque>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cassert>

#include<sstream>

#include <iostream>

#include "helper/linked_tree.hpp"
#include "helper/free_list.hpp"

template<typename T, template<typename S> class FreeListT=malloc_wrapper>
class addressable_pairing_heap
{
public:
    using elem = linked_tree<T>;
    addressable_pairing_heap() : _roots(nullptr), _last_root(nullptr), _size(0) {}
    ~addressable_pairing_heap()
    {
        while (_roots != nullptr)
        {
            auto* next = _roots->next_sibling;
            delete _roots;
            _roots = next;
        }
    }

    /// Retrieves the top element
    const T& top() const
    {
        assert(_size > 0);
        return _roots->key;
    }

    /// Add an element to the priority queue by const lvalue reference
    elem* push(const T& value)
    {
        auto* new_root = new(_free.get()) elem(value);
        insert(new_root);

        return new_root;
    }

    /// Add an element to the priority queue by rvalue reference (with move)
    elem* push(T&& value)
    {
        auto* new_root = new(_free.get()) elem(value);
        insert(new_root);

        return new_root;
    }

    /// Add an element in-place without copying or moving
    template <typename... Args>
    elem* emplace(Args&&... args)
    {
        auto* new_root = new(_free.get()) elem(std::forward<Args>(args)...);
        insert(new_root);

        return new_root;
    }

    void pop()
    {
        assert(_size > 0);
        assert(_roots != nullptr);
        assert(_last_root != nullptr);
        --_size;

        // children are new roots
        auto* child = _roots->first_child;
        _roots->first_child = nullptr;

        if (_roots == _last_root)
        {
            if (child != nullptr) child->parent = nullptr;
            _free.release(_roots);
            _roots = child;
            _last_root = child;

            // FIXME can we keep track of the last element in child lists?
            while (_last_root && _last_root->next_sibling != nullptr)
            {
                _last_root = _last_root->next_sibling;
            }
        }
        else
        {
            // append children at the end because they form
            // a single linked list
            if (child)
            {
                assert(_last_root->next_sibling == nullptr);

                child->prev_sibling = _last_root;
                _last_root->next_sibling = child;
                // _last_root is _not_ update here because we can it for free
                // in the rake function
            }

            auto* next_root = _roots->next_sibling;
            _roots->unlink_from_siblings();
            _free.release(_roots);
            _roots = next_root;

            rake_and_update_roots();
        }
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _size;
    }

    /// decreases the key of the given element
    void decrease_key(elem* element, const T& key)
    {
        assert(key <= element->key);
        element->key = key;

        if (element != _roots)
            cut_and_insert(element);
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        if (!_roots)
        {
            _roots = new_root;
            _last_root = new_root;
        }
        else if (new_root->key < _roots->key)
        {
            _roots->link_sibling_before(new_root);
            _roots = new_root;
        }
        else
        {
            _last_root->next_sibling = new_root;
            new_root->prev_sibling = _last_root;
            _last_root = new_root;
        }
        ++_size;
    }

    /// cut subtree from
    void cut_and_insert(elem* element)
    {
        assert(element != _roots);

        // is first child of the parent (also can't be a root)
        if (element->parent && element->parent->first_child == element)
        {
            auto next = element->next_sibling;
            element->parent->first_child = next;
            if (next) next->parent = element->parent;
            element->next_sibling = nullptr;
            element->prev_sibling = nullptr;

            // no need to update _last_root here
            // because element is not a root node
        }
        // could be a root or non-first child
        else
        {
            if (element == _last_root)
            {
                _last_root = element->prev_sibling;
            }
            element->unlink_from_siblings();
        }

        if (element->key < _roots->key)
        {
            assert(element->next_sibling == nullptr);
            assert(element->prev_sibling == nullptr);

            _roots->link_sibling_before(element);
            _roots = element;
        }
        else
        {
            assert(element->next_sibling == nullptr);
            assert(element->prev_sibling == nullptr);

            // append at end and update root
            _last_root->next_sibling = element;
            element->prev_sibling = _last_root;
            _last_root = element;
        }
    }

    /// Merges adjacent roots and updates:
    /// _roots if there is a new min root
    /// _last_root if the last root changed
    ///
    /// After modifying the roots _always_ call this function
    /// to get into a consistent state.
    void rake_and_update_roots()
    {
        auto* even_root = _roots;

        // unrolled because we need to update _roots
        if (even_root != nullptr && even_root->next_sibling != nullptr)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            if (odd_root->key > even_root->key)
            {
                even_root->next_sibling = next_even_root;
                if (next_even_root != nullptr)
                    next_even_root->prev_sibling = even_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                even_root->link_child(odd_root);
            }
            else
            {
                auto* prev_root = even_root->prev_sibling;
                odd_root->prev_sibling = prev_root;
                if (prev_root != nullptr)
                    prev_root->next_sibling = odd_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                odd_root->link_child(even_root);
                _roots = odd_root;
            }

            even_root = next_even_root;
        }

        // _roots is still the minimum of the first two roots
        auto* min_root = _roots;
        auto* prev_root = _roots;
        while (even_root != nullptr && even_root->next_sibling != nullptr)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            // TODO this can be done with less operations
            if (odd_root->key > even_root->key)
            {
                even_root->next_sibling = next_even_root;
                if (next_even_root != nullptr)
                    next_even_root->prev_sibling = even_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                even_root->link_child(odd_root);
                prev_root = even_root;

                if (even_root->key < min_root->key)
                    min_root = even_root;
            }
            else
            {
                odd_root->prev_sibling = prev_root;
                assert(prev_root != nullptr);
                prev_root->next_sibling = odd_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                odd_root->link_child(even_root);
                prev_root = odd_root;

                if (odd_root->key < min_root->key)
                    min_root = odd_root;
            }

            even_root = next_even_root;
        }

        if (even_root != nullptr && even_root->key < min_root->key)
            min_root = even_root;

        if (prev_root != nullptr && prev_root->next_sibling != nullptr)
        {
            _last_root = prev_root->next_sibling;
        }
        else
        {
            _last_root = prev_root;
        }

        update_min(min_root);
    }

    void update_min(elem* new_min)
    {
        if (new_min == _roots)
        {
            return;
        }

        auto new_last = new_min == _last_root ? _last_root->prev_sibling : _last_root;
        new_min->unlink_from_siblings();
        _roots->link_sibling_before(new_min);
        _roots = new_min;
        _last_root = new_last;
    }

    void _dump_siblings(elem* tree) const
    {
        std::cout << "-> " << tree << std::endl;
        while (tree != nullptr && tree->next_sibling != tree)
        {
            std::cout << "(" << tree->prev_sibling << " <- " << tree << " -> " << tree->next_sibling << ")";
            tree = tree->next_sibling;
        }
        std::cout << std::endl;
    }

    void _dump_tree(elem* tree, unsigned depth=0) const
    {
        // in case of endless recursion
        assert(depth < 5);

        // check for cycle in slibing list
        auto* slow = tree;
        auto* fast = tree != nullptr ? tree->next_sibling : nullptr;
        while (slow != nullptr && fast != nullptr)
        {
            assert(slow != fast);
            slow = slow->next_sibling;
            fast = fast->next_sibling != nullptr ? fast->next_sibling->next_sibling : nullptr;
        }

        while (tree != nullptr)
        {
            std::cout << "(" << tree->key << ": ";
            _dump_tree(tree->first_child, depth+1);
            std::cout << ")";
            tree = tree->next_sibling;
        }
    }

private:
    //! free list
    FreeListT<T> _free;
    //! first element is the min root
    elem* _roots;
    elem* _last_root;
    std::size_t _size;
};

