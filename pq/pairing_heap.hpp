#pragma once

#include <deque>
#include <vector>
#include <cassert>
#include <memory>
#include <limits>

template<typename T>
class pairing_heap
{
    struct SubTree;
    struct SubTree
    {
        T key;
        SubTree* next_child;
    };

public:
    /// Retrieves the top element
    const T& top()
    {
        assert(_trees.size() > 0);
        return _trees.front();
    }

    /// Add an element to the priority queue by const lvalue reference
    void push(const T& value)
    {
        _trees.push_back(SubTree {value, nullptr, nullptr});
        if (_trees.back().key < _trees.front().key)
        {
            std::swap(_trees.back(), _trees.front());
        }
        _roots.push_back(_trees.size()-1);
    }

    /// Add an element to the priority queue by rvalue reference (with move)
    void push(T&& value)
    {
        _trees.push_back(SubTree {std::move(value), nullptr, nullptr});
        if (_trees.back().key < _trees.front().key)
        {
            std::swap(_trees.back(), _trees.front());
        }
        _roots.push_back(_trees.size()-1);
    }

    void pop()
    {
        assert(_trees.size() > 0);

        const auto& min = _trees.front();

        for (auto* current_child = min.next_child;
             current_child != nullptr;
             current_child = current_child->next_child)
        {
            _roots.push_back(current_child);
            current_child = current_child->next_child;
        }

        _trees.pop_front();

        if (_roots.size() > 0)
        {
            auto min_idx = find_min_root();
            // swap new min to front of trees array
            std::swap(_trees.front(), _trees[min_idx]);

            // note we only need to fix the offset,
            // the swap above swapped two roots
            remove_and_shift(_roots);
        }
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _trees.size();
    }

private:

    /// merges adjacent roots and updates the indices
    void rake_roots()
    {
        BOOST_ASSERT(_trees.size() > 0);

        // merge roots
        for (int current = 0; current < _trees.size(); current += 2)
        {
            const auto current_idx = _roots[current];
            const auto next_idx = _roots[current+1];
            merge(&_trees[current_idx], &_trees[next_idx]);
        }
        // fix up roots array
        for (auto dst = 1, src = 2; src < _trees.size(); src += 2, ++dst)
        {
            _roots[dst] = _roots[src];
        }

        auto new_size = _roots.size() / 2 + (_roots.size() % 2);
        _roots.resize(new_size);
    }

    /// return index of root with min value
    std::size_t find_min_root() const
    {
        auto min_idx = _roots.front();
        auto min = _trees[min_idx].key;
        for (auto iter = std::next(_roots.begin()); iter != _roots.end(); ++iter)
        {
            if (_trees[*iter].min < min)
            {
                min_idx = *iter;
                min = _trees[min_idx].key;
            }
        }

        return min_idx;
    }

    /// merges new_child into new_parent
    void merge(SubTree* new_parent, SubTree* new_child) const
    {
        // append to child list
        new_child->next_child = new_parent->next_child;
        new_parent->next_child = new_child;
    }

    /// subtracts 1 from all indices and removes the old 0
    void remove_and_shift(std::vector<std::size_t>& indices) const
    {
        int src = 0;
        int dst = 0;
        while (src < indices.size()-1)
        {
            // skip 0 entry
            src += indices[src] == 0;
            indices[dst++] = indices[src++] - 1;
        }
        if (src < indices.size() && indices[src] != 0)
        {
            indices[dst++] = indices[src++] - 1;
        }
        indices.resize(dst);
    }

private:
    // FIXME we internall save pointers.. this is not going to work
    std::deque<SubTree> _trees;
    // we can't save points because deque might reallocate
    std::vector<std::size_t> _roots;
};
