#pragma once

#include <algorithm>
#include <cstdlib>

#include <iostream>

#include "linked_tree.hpp"

/// Exploits the fact that the tree data structure contains a linked list.
/// It uses the sibling pointer to maintain a stack of unused elements.
/// Is this a turned version of the lazy-shrinking pattern.
template<typename T, unsigned SHRINKFACTOR>
class lazyshrink_free_list
{
public:
    using tree = linked_tree<T>;

    lazyshrink_free_list() : _first(nullptr), _capacity(0), _size(0) {}

    ~lazyshrink_free_list()
    {
        _size = 0;
        shrink_to_fit();
    }

    /// Removes an eleement from the free list.
    /// If no free elements are available, allocate new.
    tree* get()
    {
        _size++;

        tree* elem;
        if (_first == nullptr)
        {
            _capacity++;
            elem = new tree();
        }
        else
        {
            assert(_first != nullptr);
            elem = pop();
        }

        assert(_size <= _capacity);

        return elem;
    }

    /// Adds elem to the free list
    void release(tree* elem)
    {
        assert(_size > 0);
        _size--;
        push(elem);

        if (_size * SHRINKFACTOR < _capacity)
        {
            shrink_to_fit();
        }
    }

    /// Returns the number of free elements.
    std::size_t free() const
    {
        return _capacity - _size;
    }

    /// Removed elements so that _size == _capacity
    void shrink_to_fit()
    {
        auto* elem = _first;
        while (elem != nullptr)
        {
            auto* next = elem->next_sibling;
            delete elem;
            elem = next;
        }
        _first = elem;
        _capacity = _size;

        assert(_capacity == _size);
        assert(_first == nullptr);
    }

private:
    /// Removes element from free list and return ons of them
    tree* pop()
    {
        assert(_first != nullptr);

        tree* return_elem = _first;
        _first = return_elem->next_sibling;
        return return_elem;
    }

    void push(tree* elem)
    {
        elem->next_sibling = _first;
        _first = elem;
    }

    tree* _first;
    std::size_t _capacity;
    std::size_t _size;
};

/// Exploits the fact that the tree data structure contains a linked list.
/// It uses the sibling pointer to maintain a stack of unused elements.
template<typename T, unsigned _GROWFACTOR, unsigned _SHRINKFACTOR>
class overallocating_free_list
{
public:
    using tree = linked_tree<T>;
    static constexpr double GROWFACTOR = _GROWFACTOR/100.0;
    static constexpr double SHRINKFACTOR = _SHRINKFACTOR/100.0;

    overallocating_free_list() : _first(nullptr), _capacity(0), _size(0) {}
    ~overallocating_free_list()
    {
        while (_first != nullptr)
        {
            pop();
        }
    }

    /// Removes an eleement from the free list.
    /// If no free elements are available, allocate new.
    tree* get()
    {
        _size++;
        if (_size*GROWFACTOR > _capacity)
        {
            reserve(_size*GROWFACTOR);
        }

        assert(_size <= _capacity);
        assert(_first != nullptr);

        auto* elem = pop();

        return elem;
    }

    /// Adds elem to the free list
    void release(tree* elem)
    {
        assert(_size > 0);
        _size--;
        if (_size*SHRINKFACTOR < _capacity)
        {
            reserve(_capacity/GROWFACTOR);
        }

        push(elem);
    }

    /// Changes the number of reserved elements, but never
    /// below the size.
    void reserve(std::size_t capacity)
    {
        capacity = std::max(1UL, std::max(_size, capacity));

        while (_capacity < capacity)
        {
            auto* elem = new tree();
            push(elem);
            _capacity++;
        }
        while (_capacity > capacity)
        {
            delete pop();
            _capacity--;
        }

        assert(_capacity == capacity);
    }

    /// Returns the number of free elements.
    std::size_t free() const
    {
        return _capacity - _size;
    }

private:
    tree* pop()
    {
        assert(_first != nullptr);

        auto* elem = _first;
        _first = elem->next_sibling;
        return elem;
    }

    void push(tree* elem)
    {
        elem->next_sibling = _first;
        _first = elem;
    };

    tree* _first;
    std::size_t _capacity;
    std::size_t _size;
};

template<typename S> using free_list = overallocating_free_list<S, 150, 300>;
template<typename S> using ls_free_list = lazyshrink_free_list<S, 300>;

/// Wraps new and delte in free list benchmakrs for comparison
template<typename T>
class malloc_wrapper
{
public:
    using tree = linked_tree<T>;

    malloc_wrapper() {}

    /// Removes an eleement from the free list.
    /// If no free elements are available, allocate new.
    tree* get()
    {
        return new tree();
    }

    /// Adds elem to the free list
    void release(tree* elem)
    {
        delete elem;
    }
};
