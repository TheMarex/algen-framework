#pragma once

#include <deque>
#include <cassert>
#include <memory>
#include <algorithm>
#include <cassert>

#include <vector>

#include <iterator>

#include <iostream>

#include "helper/linked_tree.hpp"
#include "helper/free_list.hpp"

template<typename T, template<typename S> class FreeListT=malloc_wrapper>
class addressable_pairing_heap
{
public:
    using elem = linked_tree<T>;
    addressable_pairing_heap() : _min(nullptr), _size(0) {}
    ~addressable_pairing_heap()
    {
        for (auto* r : _roots)
        {
            _free.release(r);
        }
    }

    /// Retrieves the top element
    const T& top() const
    {
        assert(_size > 0);
        return _min->key;
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
        auto* child = _min->first_child;
        _min->first_child = nullptr;
        while (child != nullptr)
        {
            auto next = child->next_sibling;
            child->prev_sibling = nullptr;
            child->next_sibling = nullptr;

            // FIXME benchmark front/back
            _roots.push_back(child);
            child = next;
        }

        std::copy_if(_roots.begin(), _roots.end(), _roots.begin(), [this](elem* e) { return e != _min; });
        // last element is now invalid since we removed _min
        // and copied all other
        _roots.pop_back();
        _free.release(_min);

        if (_roots.size() > 0)
        {
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

        if (element->parent != nullptr)
        {
            cut_and_insert(element);
        }

        if (element->key < _min->key)
        {
            _min = element;
        }
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        _roots.push_back(new_root);
        if (_size == 0 || new_root->key < _min->key)
        {
            _min = new_root;
        }
        ++_size;
    }

    /// cut subtree from
    void cut_and_insert(elem* element)
    {
        assert(element->parent != nullptr);

        // is first child of the parent
        if (element->parent->first_child == element)
        {
            auto next = element->next_sibling;
            element->parent->first_child = next;
            if (next) next->parent = element->parent;
            element->next_sibling = nullptr;
            element->prev_sibling = nullptr;
        }
        else
        {
            element->unlink_from_siblings();
        }

        // FIXME benchmark front/back
        _roots.push_back(element);
    }

    /// Merges adjacent roots and updates _min
    void rake_and_update_roots()
    {
        assert(_roots.size() > 0);

        auto output_iter = _roots.begin();
        auto even_iter = _roots.begin();
        elem* new_min = nullptr;
        while (even_iter != _roots.end() && std::next(even_iter) != _roots.end())
        {
            auto odd_iter = std::next(even_iter);
            auto* even_root = *even_iter;
            auto* odd_root = *odd_iter;

            if (odd_root->key > even_root->key)
            {
                even_root->link_child(odd_root);

                if (new_min == nullptr || even_root->key < new_min->key)
                    new_min = even_root;

                *output_iter = *even_iter;
            }
            else
            {
                odd_root->link_child(even_root);

                if (new_min == nullptr || odd_root->key < new_min->key)
                    new_min = odd_root;

                *output_iter = *odd_iter;
            }

            ++output_iter;
            even_iter = std::next(odd_iter);
        }

        // handle the last root
        if (even_iter != _roots.end())
        {
            if (new_min == nullptr || (*even_iter)->key < new_min->key)
                new_min = *even_iter;

            *output_iter = *even_iter;
            ++output_iter;
        }

        _min = new_min;

        _roots.resize(output_iter - _roots.begin());
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
    std::vector<elem*> _roots;
    elem* _min;
    std::size_t _size;
};

