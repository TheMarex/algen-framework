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

    addressable_pairing_heap() : _top(nullptr), _size(0) {}
    ~addressable_pairing_heap()
    {
        for (auto* r : _roots)
        {
            _free.release(r);
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

        // remove _top from _roots
        std::copy_if(_roots.begin(), _roots.end(), _roots.begin(), [this](elem* root) { return root != _top; });
        _roots.pop_back();

        // children are new roots
        auto* child = _top->first_child;
        _top->first_child = nullptr;
        while (child != nullptr)
        {
            auto next = child->next_sibling;
            child->prev_sibling = nullptr;
            child->next_sibling = nullptr;

            // FIXME benchmark front/back
            _roots.push_back(child);
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

        if (element->parent != nullptr)
        {
            element->unlink_from_parent();
            _roots.push_back(element);
        }
        element->key = key;
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

        // is a root element
        if (element->parent == nullptr)
        {
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
                    _roots.push_back(child);
                }
            }
        }
        else
        {
            auto* grandfather = element->parent;
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
                    grandfather->link_child(child);
                }
            }

            element->unlink_from_parent();
            _roots.push_back(element);
        }

        _top = nullptr;

        LOG_STATE("< modify");
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        _roots.push_back(new_root);
        _top = nullptr;
        ++_size;
    }

    /// Merges adjacent roots and updates _top
    void update_top()
    {
        LOG_STATE("> update_top");

        assert(_roots.size() > 0);

        auto output_iter = _roots.begin();
        auto even_iter = _roots.begin();
        auto odd_iter = even_iter == _roots.end() ? _roots.end() : std::next(even_iter);
        _top = nullptr;
        while (odd_iter != _roots.end())
        {
            auto* even_root = *even_iter;
            auto* odd_root = *odd_iter;

            if (_cmp(odd_root->key, even_root->key))
            {
                even_root->link_child(odd_root);

                if (_top == nullptr || _cmp(_top->key, even_root->key))
                    _top = even_root;

                *output_iter = *even_iter;
            }
            else
            {
                odd_root->link_child(even_root);

                if (_top == nullptr || _cmp(_top->key, odd_root->key))
                    _top = odd_root;

                *output_iter = *odd_iter;
            }

            ++output_iter;
            even_iter = std::next(odd_iter);
            odd_iter = even_iter == _roots.end() ? _roots.end() : std::next(even_iter);
        }

        // handle the last root
        if (even_iter != _roots.end())
        {
            if (_top == nullptr || _cmp(_top->key, (*even_iter)->key))
                _top = *even_iter;

            *output_iter = *even_iter;
            ++output_iter;
        }

        auto new_size = std::distance(_roots.begin(), output_iter);
        _roots.resize(new_size);

        LOG_STATE("< update_top");
        LOG_MSG("  top: " << _top << "(" << _top->key << ")");
    }

    void _dump_state(const char* prefix) const
    {
        std::cout << prefix << " : ";
        std::for_each(_roots.begin(), _roots.end(), [this](elem* e) {_dump_tree(e); std::cout << ", "; });
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
    elem* _top;
    Compare _cmp;
    std::size_t _size;
};

