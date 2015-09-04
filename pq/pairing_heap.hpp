#pragma once

#include <vector>
#include <cassert>
#include <memory>

template<typename T>
class pairing_heap
{
    struct HeapElem;
    struct HeapElem
    {
        HeapElem() : is_empty(true) {}
        HeapElem(T&& key) : key(std::move(key)), is_empty(false) {}
        HeapElem(HeapElem&& other)
        {
            key = std::move(other.key);
            sub_heaps = std::move(other.sub_heaps);
        }
        ~HeapElem()
        {
            for (auto* elem : sub_heaps)
            {
                delete elem;
            }
        }
        void operator=(HeapElem&& other)
        {
            key = std::move(other.key);
            sub_heaps = std::move(other.sub_heaps);
        }

        T key;
        bool is_empty;
        std::vector<HeapElem*> sub_heaps;
    };

public:
    /// Retrieves the top element
    const T& top()
    {
        assert(_size > 0);
        return _root.key;
    }

    /// Add an element to the priority queue by const lvalue reference
    void push(const T& value)
    {
        ++_size;
        auto new_heap = new HeapElem(value);
        merge(_root, new_heap);
    }

    /// Add an element to the priority queue by rvalue reference (with move)
    void push(T&& value)
    {
        ++_size;
        HeapElem new_heap;
        new_heap.key = std::move(value);
        new_heap.is_empty = false;
        merge(_root, std::move(new_heap));
    }

    void pop()
    {
        assert(_size > 0);
        --_size;
        replace_root(_root);
    }

    /// Get the number of elements in the priority queue
    std::size_t size() const
    {
        return _size;
    }

private:

    /// Appends rhs to lhs
    /// This does not update the global _size, so we can't make it public.
    void merge(HeapElem& lhs, HeapElem* rhs) const
    {
        if (lhs.is_empty)
        {
            lhs = std::move(*rhs);
            delete rhs;
            return;
        }
        else if (rhs->is_empty)
        {
            delete rhs;
            return;
        }

        if (lhs.key > rhs->key)
        {
            std::swap(lhs.key, rhs->key);
            lhs.sub_heaps.swap(rhs->sub_heaps);
        }
        lhs.sub_heaps.push_back(rhs);
    }

    /// Implements the merge_pairs function in a non-recursive form
    void replace_root(HeapElem& root) const
    {
        auto n = root.sub_heaps.size();

        if (n == 0U)
        {
            root.is_empty = true;
            return;
        }

        if (n > 1U)
        {
            // Merge pairs of two heaps
            // H0 H1 H2 H3 H4 H5 H6
            // ^---- ^---- ^----
            for (auto i = 1u; i < n; i += 2)
            {
                merge(root.sub_heaps[i-1], root.sub_heaps[i]);
            }

            // If there is an odd number of heaps, we left one out
            // H0 H1 H2 H3 H4 H5 H6
            // ^---- ^---- ^----
            //             ^------
            if (n % 2 != 0)
            {
                assert(n >= 3);
                merge(root.sub_heaps[n-3], root.sub_heaps.back());
                --n;
            }

            assert(n % 2 == 0);

            // Now pair-wise merge to front
            // H0 H1 H2 H3 H4 H5
            // ^---- ^---- ^----
            //       ^-------
            // ^-------
            for (auto i = n-1; i > 3; i -= 2)
            {
                merge(root.sub_heaps[i-3], root.sub_heaps[i-1]);
            }
        }

        root.reset(root.sub_heaps.front());
    }

private:
    std::unique_ptr<HeapElem> _root;
    unsigned _size;
};
