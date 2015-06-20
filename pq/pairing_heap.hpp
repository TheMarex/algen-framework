#pragma once

#include <vector>

template<typename T>
class pairing_heap
{
    struct HeapElem;
    struct HeapElem
    {
        HeapElem() : is_empty(true) {}
        HeapElem(HeapElem&& other)
        {
            key = std::move(other.key);
            sub_heaps = std::move(other.sub_heaps);
        }

        T key;
        bool is_empty;
        std::vector<HeapElem*> sub_heaps;
    };


    /// Retrieves the top element
    const T& top()
    {
        assert(_size > 0);
        return _root.key;
    }

    /// Add an element to the priority queue by const lvalue reference
    void push(const T& value)
    {
        HeapElem new_heap;
        new_heap.key = value;
        new_heap.is_empty = false;
        merge(_root, std::move(new_heap));
    }

    /// Add an element to the priority queue by rvalue reference (with move)
    void push(T&& value)
    {
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
    void merge(HeapElem& lhs, HeapElem&& rhs) const
    {
        if (lhs.is_empty)
        {
            lhs = std::move(rhs);
        }
        else if (rhs.is_empty)
        {
            return;
        }

        // make sure that we always copy the least amount of elements
        if (lhs.sub_heaps.size() < rhs.sub_heaps.size())
        {
            lhs.sub_heaps.swap(rhs.sub_heaps);
        }

        lhs.sub_heaps.insert(lhs.sub_heaps.end(), rhs.sub_heaps.begin(), rhs.sub_heaps.end());

        if (lhs.key > rhs.key)
        {
            std::swap(lhs.key, rhs.key);
        }
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

        root = std::move(root.sub_heaps.front());
    }

private:
    HeapElem _root;
    unsigned _size;
};
