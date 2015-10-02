#pragma once

#include <deque>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cassert>

#include<sstream>

#include <iostream>

namespace list_helper
{
    template<typename T>
    bool is_valid(T* begin)
    {
        auto* current = begin;
        while (current != nullptr)
        {
            if (current->prev_sibling &&
                current->prev_sibling->next_sibling != current)
                return false;
            if (current->next_sibling &&
                current->next_sibling->prev_sibling != current)
                return false;

            current = current->next_sibling;
            assert(!current || current != current->next_sibling);
        }

        return true;
    }

    template<typename T>
    void link_sibling(T* after_elem, T* new_elem)
    {
        assert(after_elem != nullptr);
        assert(new_elem != nullptr);

        // link up next elem
        auto* new_next = after_elem->next_sibling;
        if (new_next) new_next->prev_sibling = new_elem;
        new_elem->next_sibling = new_next;

        // link up prev elem
        after_elem->next_sibling = new_elem;
        new_elem->prev_sibling = after_elem;

        assert(is_valid(after_elem));
    }

    template<typename T>
    void link_before_sibling(T* before_elem, T* new_elem)
    {
        assert(before_elem != nullptr);
        assert(new_elem != nullptr);

        // link up next elem
        auto* new_prev = before_elem->prev_sibling;
        if (new_prev) new_prev->next_sibling = new_elem;
        new_elem->prev_sibling = new_prev;

        // link up prev elem
        before_elem->prev_sibling = new_elem;
        new_elem->next_sibling = before_elem;

        assert(is_valid(before_elem));
    }

    template<typename T>
    void unlink_silbling(T* elem)
    {
        auto* old_prev = elem->prev_sibling;
        auto* old_next = elem->next_sibling;

        if (old_prev) old_prev->next_sibling = old_next;
        if (old_next) old_next->prev_sibling = old_prev;

        elem->prev_sibling = nullptr;
        elem->next_sibling = nullptr;

        assert(elem->prev_sibling == nullptr &&
               elem->next_sibling == nullptr);

        assert(is_valid(old_prev));
    }

    template<typename T>
    void link_child(T* parent, T* new_elem)
    {
        new_elem->parent = parent;

        // link to first child
        if (parent->first_child)
        {
            parent->first_child->prev_sibling = new_elem;
            new_elem->next_sibling = parent->first_child;
        }

        parent->first_child = new_elem;
    }
}

template<typename T>
class naive_pairing_heap
{
    struct SubTree;
    struct SubTree
    {
        SubTree(const T& in_key)
            : key(in_key), prev_sibling(nullptr), next_sibling(nullptr), first_child(nullptr) {}
        SubTree(T&& in_key)
            : key(std::move(in_key)), prev_sibling(nullptr), next_sibling(nullptr), first_child(nullptr){}

        ~SubTree()
        {
            // deallocate all children.
            for (auto* child = first_child; child != nullptr; child = child->next_sibling)
            {
                delete child;
            }
        }

        T key;
        //! the first child stores the parent here
        union {
            SubTree* prev_sibling;
            SubTree* parent;
        };
        SubTree* next_sibling;
        SubTree* first_child;
    };

public:
    naive_pairing_heap() : _roots(nullptr), _size(0) {}

    /// Retrieves the top element
    const T& top()
    {
        assert(_size > 0);
        return _roots->key;
    }

    /// Add an element to the priority queue by const lvalue reference
    SubTree* push(const T& value)
    {
        auto* new_root = new SubTree(value);
        if (!_roots)
        {
            _roots = new_root;
        }
        else if (value < _roots->key)
        {
            list_helper::link_before_sibling(_roots, new_root);
            _roots = new_root;
        }
        else
        {
            list_helper::link_sibling(_roots, new_root);
        }
        ++_size;

        return new_root;
    }

    /// Add an element to the priority queue by rvalue reference (with move)
    SubTree* push(T&& value)
    {
        auto* new_root = new SubTree(value);
        if (!_roots)
        {
            _roots = new_root;
        }
        else if (value < _roots->key)
        {
            list_helper::link_before_sibling(_roots, new_root);
            _roots = new_root;
        }
        else
        {
            list_helper::link_sibling(_roots, new_root);
        }
        ++_size;

        return new_root;
    }

    void pop()
    {
        assert(_size > 0);
        --_size;

        // children are new roots
        auto* first_child = _roots->first_child;
        if (first_child)
        {
            _roots->first_child = nullptr;
            first_child->parent = nullptr;
            while (first_child != nullptr)
            {
                auto* next = first_child->next_sibling;
                list_helper::unlink_silbling(first_child);
                list_helper::link_sibling(_roots, first_child);
                first_child = next;
            }
        }

        auto* next_root = _roots->next_sibling;
        list_helper::unlink_silbling(_roots);
        delete _roots;
        _roots = next_root;

        update_min();

        rake_roots();
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _size;
    }

    /// decreases the key of the given element
    void decrease_key(SubTree* element, const T& key)
    {
        element->key = key;
        cut_and_insert(element);

        update_min();
    }

private:
    /// cut subtree from
    void cut_and_insert(SubTree* element)
    {
        list_helper::unlink_silbling(element);
        // might not be the best position to insert
        list_helper::link_sibling(_roots, element);
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
            list_helper::unlink_silbling(min_root);
            list_helper::link_before_sibling(_roots, min_root);
            _roots = min_root;
        }
    }

    /// merges adjacent roots
    void rake_roots()
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
                list_helper::unlink_silbling(odd_root);
                list_helper::link_child(even_root, odd_root);
            }
            else
            {
                list_helper::unlink_silbling(even_root);
                list_helper::link_child(odd_root, even_root);
                _roots = odd_root;
            }

            even_root = next_even_root;
        }

        while (even_root != nullptr && even_root->next_sibling != nullptr)
        {
            auto* odd_root = even_root->next_sibling;
            auto* next_even_root = odd_root->next_sibling;

            // TODO this can be done with less operations
            if (odd_root->key > even_root->key)
            {
                list_helper::unlink_silbling(odd_root);
                list_helper::link_child(even_root, odd_root);
            }
            else
            {
                list_helper::unlink_silbling(even_root);
                list_helper::link_child(odd_root, even_root);
            }

            even_root = next_even_root;
        }
    }

    void _dump_siblings(SubTree* tree) const
    {
        std::cout << "-> " << tree << std::endl;
        while (tree != nullptr && tree->next_sibling != tree)
        {
            std::cout << "(" << tree->prev_sibling << " <- " << tree << " -> " << tree->next_sibling << ")";
            tree = tree->next_sibling;
        }
        std::cout << std::endl;
    }

    void _dump_tree(SubTree* tree) const
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
    //! first element is the min root
    SubTree* _roots;
    std::size_t _size;
};

