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
    addressable_pairing_heap() : _roots(nullptr), _size(0) {}
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
        --_size;

        // children are new roots
        auto* child = _roots->first_child;
        if (child)
        {
            _roots->first_child = nullptr;
            child->parent = nullptr;
            while (child != nullptr)
            {
                auto* next = child->next_sibling;
                child->unlink_from_siblings();
                _roots->link_sibling(child);
                child = next;
            }
        }

        auto* next_root = _roots->next_sibling;
        _roots->unlink_from_siblings();
        _free.release(_roots);
        _roots = next_root;

        rake_and_update_roots();
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _size;
    }

    /// decreases the key of the given element
    void decrease_key(elem* element, const T& key)
    {
        element->key = key;

        cut_and_insert(element);

        rake_and_update_roots();
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        if (!_roots)
        {
            _roots = new_root;
        }
        else if (new_root->key < _roots->key)
        {
            _roots->link_sibling_before(new_root);
            _roots = new_root;
        }
        else
        {
            _roots->link_sibling(new_root);
        }
        ++_size;
    }

    /// cut subtree from
    void cut_and_insert(elem* element)
    {
        // is first child of the parent (also can't be a root)
        if (element->parent && element->parent->first_child == element)
        {
            auto next = element->next_sibling;
            element->parent->first_child = next;
            if (next) next->parent = element->parent;
            element->parent->first_child = next;
        }
        // is a root or non-first child
        else
        {
            element->unlink_from_siblings();
        }
        // might not be the best position to insert
        _roots->link_sibling(element);
    }

    /// scans the roots for the minium and places it in the front
    void update_min()
    {
        auto* min_root = _roots;
        auto* current_root = _roots;
        while (current_root != nullptr)
        {
            if (current_root->key < min_root->key)
            {
                min_root = current_root;
            }
            current_root = current_root->next_sibling;
        }

        if (min_root != _roots)
        {
            min_root->unlink_from_siblings();
            _roots->link_sibling_before(min_root);
            _roots = min_root;
        }
    }

    /// merges adjacent roots
    void rake_and_update_roots()
    {
        auto* even_root = _roots;

        // unrolled because we need to update _roots
        if (even_root != nullptr && even_root->next_sibling != nullptr)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            // TODO this can be done with less operations
            if (odd_root->key > even_root->key)
            {
                odd_root->unlink_from_siblings();
                even_root->link_child(odd_root);
            }
            else
            {
                even_root->unlink_from_siblings();
                odd_root->link_child(even_root);
                _roots = odd_root;
            }

            even_root = next_even_root;
        }

        // _roots is still the minimum of the first two roots
        auto* min_root = _roots;

        while (even_root != nullptr && even_root->next_sibling != nullptr)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            // TODO this can be done with less operations
            if (odd_root->key > even_root->key)
            {
                odd_root->unlink_from_siblings();
                even_root->link_child(odd_root);
                if (even_root->key < min_root->key)
                    min_root = even_root;
            }
            else
            {
                even_root->unlink_from_siblings();
                odd_root->link_child(even_root);
                if (odd_root->key < min_root->key)
                    min_root = odd_root;
            }

            even_root = next_even_root;
        }

        if (min_root != _roots)
        {
            min_root->unlink_from_siblings();
            _roots->link_sibling_before(min_root);
            _roots = min_root;
        }
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

    void _dump_tree(elem* tree) const
    {
        while (tree != nullptr && tree->next_sibling != tree)
        {
            std::cout << "(" << tree->key << ": ";
            _dump_tree(tree->first_child);
            std::cout << ")";
            tree = tree->next_sibling;
        }
    }

private:
    //! free list
    FreeListT<T> _free;
    //! first element is the min root
    elem* _roots;
    std::size_t _size;
};

