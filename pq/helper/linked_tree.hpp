#pragma once

#include <memory>

/// Represents an n-ary tree using two linked lists
/// Siblings are kept in a linked list. The parent of all children
/// is the first element in the children linked list.
/// Parents own children.
template<typename T> struct linked_sub_tree;
template<typename T>
struct linked_tree
{
    using tree = linked_tree<T>;

    linked_tree(const T& in_key)
        : key(in_key) {}

    linked_tree(T&& in_key)
        : key(std::move(in_key)) {}

    template<typename... Args>
        linked_tree(Args&&... args)
        : key(std::forward<Args>(args)...) {}

    ~linked_tree()
    {
        // deallocate all children.
        for (auto* child = first_child; child != nullptr; child = child->next_sibling)
        {
            delete child;
        }
    }

    /// checks if the current siblings list is valid
    bool is_valid() const
    {
        auto* current = this;
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

    /// Links elem after this elementts
    void link_sibling(tree* elem)
    {
        assert(elem != nullptr);

        auto* new_next = next_sibling;
        if (new_next) new_next->prev_sibling = elem;
        next_sibling = elem;

        elem->next_sibling = new_next;
        elem->prev_sibling = this;

        assert(is_valid());
    }

    /// Links elem before this element
    void link_sibling_before(tree* elem)
    {
        assert(elem != nullptr);

        // link up next elem
        auto* new_prev = prev_sibling;
        if (new_prev) new_prev->next_sibling = elem;
        prev_sibling = elem;

        elem->prev_sibling = new_prev;
        elem->next_sibling = this;

        assert(is_valid());
    }

    /// Removes element form siblings list
    void unlink_from_siblings()
    {
        auto* old_prev = prev_sibling;
        auto* old_next = next_sibling;

        if (old_prev) old_prev->next_sibling = old_next;
        if (old_next) old_next->prev_sibling = old_prev;

        prev_sibling = nullptr;
        next_sibling = nullptr;

        assert(prev_sibling == nullptr &&
               next_sibling == nullptr);

        assert(old_prev->is_valid());
    }

    void link_child(tree* new_elem)
    {
        new_elem->parent = this;

        // link to first child
        if (first_child)
        {
            first_child->prev_sibling = new_elem;
            new_elem->next_sibling = first_child;
        }

        first_child = new_elem;
    }

    T key;
    //! the first child stores the parent here
    union {
        tree* prev_sibling = nullptr;
        tree* parent;
    };
    tree* next_sibling = nullptr;
    tree* first_child = nullptr;
};

