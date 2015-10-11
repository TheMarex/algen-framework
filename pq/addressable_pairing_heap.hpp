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
    const T& top() const
    {
        assert(_size > 0);
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

        assert(_size > 0);
        --_size;

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

        LOG_STATE("> rake");
        auto old_top = _top;
        rake_and_update_roots(old_top);
        // last element is now invalid since we removed _top
        // and copied all other
        _free.release(old_top);

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

        if (_cmp(_top->key, element->key))
        {
            _top = element;
        }
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

            rake_and_update_roots(nullptr);
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
            if (_cmp(_top->key, element->key))
            {
                _top = element;
            }
        }

        LOG_STATE("< modify");
    }

private:
    /// insert new element into heap
    void insert(elem* new_root)
    {
        _roots.push_back(new_root);
        if (_size == 0 || _cmp(_top->key, new_root->key))
        {
            _top = new_root;
        }
        ++_size;
    }

    /// Merges adjacent roots and updates _top
    void rake_and_update_roots(elem* old_top)
    {
        assert(_roots.size() > 0);

        auto output_iter = _roots.begin();
        auto even_iter = _roots.begin();
        auto odd_iter = even_iter == _roots.end() ? _roots.end() : std::next(even_iter);
        elem* new_top = nullptr;
        while (odd_iter != _roots.end())
        {
            if (*even_iter == old_top)
            {
                even_iter = odd_iter;
                odd_iter = std::next(odd_iter);
                continue;
            }
            else if (*odd_iter == old_top)
            {
                odd_iter = std::next(odd_iter);
                continue;
            }
            auto* even_root = *even_iter;
            auto* odd_root = *odd_iter;

            if (_cmp(odd_root->key, even_root->key))
            {
                even_root->link_child(odd_root);

                if (new_top == nullptr || _cmp(new_top->key, even_root->key))
                    new_top = even_root;

                *output_iter = *even_iter;
            }
            else
            {
                odd_root->link_child(even_root);

                if (new_top == nullptr || _cmp(new_top->key, odd_root->key))
                    new_top = odd_root;

                *output_iter = *odd_iter;
            }

            ++output_iter;
            even_iter = std::next(odd_iter);
            odd_iter = even_iter == _roots.end() ? _roots.end() : std::next(even_iter);
        }

        // handle the last root
        if (even_iter != _roots.end() && *even_iter != old_top)
        {
            if (new_top == nullptr || _cmp(new_top->key, (*even_iter)->key))
                new_top = *even_iter;

            *output_iter = *even_iter;
            ++output_iter;
        }

        _top = new_top;

        auto new_size = std::distance(_roots.begin(), output_iter);
        _roots.resize(new_size);

        assert(std::all_of(_roots.begin(), _roots.end(), [old_top](elem* e) { return e != old_top; }));
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

