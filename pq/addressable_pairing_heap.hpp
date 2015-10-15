#pragma once

#include <deque>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cassert>

#include <vector>

#include <iterator>
#include <functional>

#include <iostream>

#include "helper/linked_tree.hpp"
#include "helper/free_list.hpp"

#define LOGGING 0
#define LOG_STATE(_X) if (LOGGING) { _dump_state(_X); }
#define LOG_MSG(_X) if (LOGGING) { std::cout << _X << std::endl; }

template<typename T, class Compare = std::less<T>, template<typename S> class FreeListT=malloc_wrapper>
class addressable_pairing_heap
{
public:
    using elem = linked_tree<T>;
    using handle_type = elem*;
    using comparator_type = Compare;

    addressable_pairing_heap() : _top(nullptr), _size(0)
    {
        // make linked list circular
        _roots.next_sibling = &_roots;
        _roots.prev_sibling = &_roots;
    }

    ~addressable_pairing_heap()
    {
        auto* tree = _roots.next_sibling;
        while (tree != &_roots)
        {
            auto* next = tree->next_sibling;
            _free.release(tree);
            tree = next;
        }
    }

    /// Retrieves the top element
    const T& top()
    {
        if (_top == nullptr)
        {
            update_top();
        }
        assert(_top != nullptr);

        return _top->key;
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
        LOG_STATE("> push");

        auto* new_root = new(_free.get()) elem(value);
        insert(new_root);

        LOG_STATE("< push");

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
        LOG_STATE("> pop");

        if (_top == nullptr)
        {
            update_top();
        }
        assert(_top != nullptr);
        assert(_size > 0);
        --_size;

        // remove _top from linked list
        _top->next_sibling->prev_sibling = _top->prev_sibling;
        _top->prev_sibling->next_sibling = _top->next_sibling;

        // children are new roots
        auto* child = _top->first_child;
        _top->first_child = nullptr;
        while (child != nullptr)
        {
            auto next = child->next_sibling;
            child->prev_sibling = nullptr;
            child->next_sibling = nullptr;

            append_root(child);
            child = next;
        }

        _free.release(_top);
        _top = nullptr;

        LOG_STATE("< pop");
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _size;
    }

    /// Modifies the key of the given element so that it moves towards the top
    /// Only supports updates with Compare(old_key, new_key)
    void modify_up(elem* element, const T& key)
    {
        LOG_STATE("> modify_up");
        assert(_cmp(element->key, key) || element->key == key);

        element->key = key;

        // this even works for roots because of the shared prev/parent ptr
        element->unlink_from_parent();
        append_root(element);
        _top = nullptr;

        LOG_STATE("< modify_up");
    }

    /// modifies the key of the given element
    void modify(elem* element, const T& key)
    {
        LOG_STATE("> modify");

        if (_cmp(element->key, key))
        {
            modify_up(element, key);
            return;
        }
        assert(!_cmp(element->key, key));
        element->key = key;

        for (elem *child = element->first_child, *next = nullptr;
             child != nullptr;
             child = next)
        {
            next = child->next_sibling;

            // heap property violated
            // move child up to parent
            if (_cmp(element->key, child->key))
            {
                child->unlink_from_parent();
                append_root(child);
            }
        }

        element->unlink_from_parent();
        append_root(element);
        _top = nullptr;

        LOG_STATE("< modify");
    }

    comparator_type& get_comparator()
    {
        return _cmp;
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        append_root(new_root);

        _top = nullptr;
        ++_size;
    }

    /// Appends a new root at the end
    /// Make sure to invalidate _top afterwards
    void append_root(elem* new_root)
    {
        assert(_roots.prev_sibling->next_sibling == &_roots);

        _roots.prev_sibling->next_sibling = new_root;
        new_root->prev_sibling = _roots.prev_sibling;
        new_root->next_sibling = &_roots;
        _roots.prev_sibling = new_root;

        assert(_roots.prev_sibling->next_sibling == &_roots);
    }

    /// Merges adjacent roots and updates _top
    void update_top()
    {
        LOG_STATE("> update_top");
        assert(_top == nullptr);

        auto* even_root = _roots.next_sibling;
        while (even_root != &_roots && even_root->next_sibling != &_roots)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            if (_cmp(odd_root->key, even_root->key))
            {
                even_root->next_sibling = next_even_root;
                next_even_root->prev_sibling = even_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                even_root->link_child(odd_root);

                if (_top == nullptr || _cmp(_top->key, even_root->key))
                    _top = even_root;
            }
            else
            {
                odd_root->prev_sibling = even_root->prev_sibling;
                even_root->prev_sibling->next_sibling = odd_root;

                // prev_sibling and next_sibling is always overwritten by link_child
                odd_root->link_child(even_root);

                if (_top == nullptr || _cmp(_top->key, odd_root->key))
                    _top = odd_root;
            }

            even_root = next_even_root;
        }

        if (even_root != &_roots && (_top == nullptr || _cmp(_top->key, even_root->key)))
        {
            _top = even_root;
        }

        LOG_STATE("< update_top");
        LOG_MSG("  top: " << _top << "(" << _top->key << ")");
    }

    void _dump_state(const char* prefix) const
    {
        std::cout << prefix << " : ";
        auto* tree = _roots.next_sibling;
        while (tree != &_roots)
        {
            std::cout << "(" << tree->key << ": ";
            _dump_tree(tree->first_child);
            std::cout << "), ";
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
    //! dual linked list of roots, this is the senitel
    elem _roots;
    //! lazy-updated pointer to top element, nullptr if not set
    elem* _top;
    comparator_type _cmp;
    std::size_t _size;
};

