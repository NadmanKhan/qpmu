#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>

namespace qpmu {
namespace detail {

// Flip the order of a binary operator
template <typename Binary_Operator>
struct Flip
{
    Binary_Operator op;

    template <typename T>
    constexpr auto operator()(const T &a, const T &b) const
    {
        return op(b, a);
    }
};

// Fast floor(log2(n)) for n > 0
constexpr std::size_t floor_log2(std::size_t n) noexcept
{
    return n <= 1 ? 0
#if defined(__GNUC__) || defined(__clang__)
                  : (sizeof(unsigned long long) * 8 - 1) - __builtin_clzll(n);
#else
            : n >= 0x100000000 ? 32 + floor_log2(n >> 32)
            : n >= 0x10000     ? 16 + floor_log2(n >> 16)
            : n >= 0x100       ? 8 + floor_log2(n >> 8)
            : n >= 0x10        ? 4 + floor_log2(n >> 4)
            : n >= 0x4         ? 2 + floor_log2(n >> 2)
                               : 1 + floor_log2(n >> 1);
#endif
}

constexpr bool in_odd_level(std::size_t n) noexcept
{
    return bool(floor_log2(n + 1) & 1);
}

// Parent index in a binary heap indexed from 0 in BFS order (root at 0)
constexpr std::size_t index_parent(std::size_t i) noexcept
{
    return (i - 1) / 2;
}

// Left child index in a binary heap indexed from 0 in BFS order (root at 0)
constexpr std::size_t index_left_child(std::size_t i) noexcept
{
    return (i * 2) + 1;
}

// Right child index in a binary heap indexed from 0 in BFS order (root at 0)
constexpr std::size_t index_right_child(std::size_t i) noexcept
{
    return (i * 2) + 2;
}

template <typename Iterator>
inline void swap_iterator_values(Iterator i, Iterator j) noexcept
{
    using std::iter_swap; // Allow argument-dependent lookup (ADL) to find custom iter_swap so that
                          // position tracking is possible
    iter_swap(i, j);
}

template <typename Random_Access_Iterator, typename Level_Type_Specific_Compare>
Random_Access_Iterator
trickle_down_iter(Random_Access_Iterator first,
                  typename std::iterator_traits<Random_Access_Iterator>::difference_type len,
                  Random_Access_Iterator pos, const Level_Type_Specific_Compare &should_be_above)
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    while (true) {
        Difference_Type pos_index = pos - first;

        auto best = pos;
        for (Difference_Type d : { index_left_child(pos_index), index_right_child(pos_index),
                                   index_left_child(index_left_child(pos_index)),
                                   index_right_child(index_left_child(pos_index)),
                                   index_left_child(index_right_child(pos_index)),
                                   index_right_child(index_right_child(pos_index)) }) {
            if (d >= len)
                break;
            if (should_be_above(*(first + d), *best)) {
                best = (first + d);
            }
        }
        Difference_Type best_index = best - first;

        if (best_index == pos_index)
            return pos;

        swap_iterator_values(best, pos);

        if (best_index == Difference_Type(index_left_child(pos_index))
            || best_index == Difference_Type(index_right_child(pos_index)))
            return best;

        auto parent_of_best = first + Difference_Type(index_parent(best_index));
        if (should_be_above(*parent_of_best, *best)) {
            swap_iterator_values(parent_of_best, best);
            trickle_down_iter(first, len, best, should_be_above);
            return parent_of_best;
        }

        pos = best;
    }
}

template <typename Random_Access_Iterator, typename Level_Type_Specific_Compare>
void bubble_up_iter(Random_Access_Iterator first, Random_Access_Iterator pos,
                    const Level_Type_Specific_Compare &should_be_above)
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    while (true) {
        auto pos_index = pos - first;
        if (pos_index < 3)
            return;

        auto grandparent = first + Difference_Type(index_parent(index_parent(pos_index)));
        if (should_be_above(*pos, *grandparent)) {
            swap_iterator_values(pos, grandparent);
            pos = grandparent;
        } else {
            return;
        }
    }
}

template <typename Random_Access_Iterator, typename Compare>
Random_Access_Iterator
trickle_down(Random_Access_Iterator first,
             typename std::iterator_traits<Random_Access_Iterator>::difference_type len,
             Random_Access_Iterator pos, const Compare &comp_less,
             const Flip<Compare> &comp_greater)
{
    if (detail::in_odd_level(pos - first)) {
        return detail::trickle_down_iter(first, len, pos, comp_greater);
    } else {
        return detail::trickle_down_iter(first, len, pos, comp_less);
    }
}

template <typename Random_Access_Iterator, typename Compare>
void bubble_up(Random_Access_Iterator first, Random_Access_Iterator pos, const Compare &comp_less,
               const Flip<Compare> &comp_greater)
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    if (pos == first)
        return;
    Difference_Type pos_index = pos - first;
    auto parent = first + Difference_Type(detail::index_parent(pos_index));
    if (detail::in_odd_level(pos_index)) {
        const auto &comp_node_level = comp_greater;
        const auto &comp_parent_level = comp_less;
        if (comp_node_level(*parent, *pos)) {
            swap_iterator_values(pos, parent);
            bubble_up_iter(first, parent, comp_parent_level);
        } else {
            bubble_up_iter(first, pos, comp_node_level);
        }
    } else {
        const auto &comp_node_level = comp_less;
        const auto &comp_parent_level = comp_greater;
        if (comp_node_level(*parent, *pos)) {
            swap_iterator_values(pos, parent);
            bubble_up_iter(first, parent, comp_parent_level);
        } else {
            bubble_up_iter(first, pos, comp_node_level);
        }
    }
}

} // namespace detail

template <typename Random_Access_Iterator, typename Compare = std::less<>>
void make_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                      const Compare &comp = Compare{})
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    Difference_Type len = last - first;
    detail::Flip<Compare> flip_comp = { comp };
    if (len > 1) {
        // start from the first parent, there is no need to consider children
        for (Difference_Type start = (len - 2) / 2; start >= 0; --start) {
            detail::trickle_down(first, len, first + start, comp, flip_comp);
        }
    }
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
void update_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                        Random_Access_Iterator pos, const Compare &comp = Compare{})
{
    static_assert(std::is_copy_constructible<Random_Access_Iterator>::value,
                  "Iterators must be copy constructible.");
    static_assert(std::is_copy_assignable<Random_Access_Iterator>::value,
                  "Iterators must be copy assignable.");

    detail::Flip<Compare> flip_comp = { comp };
    auto final_pos = detail::trickle_down(first, last - first, pos, comp, flip_comp);
    detail::bubble_up(first, final_pos, comp, flip_comp);
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
void push_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                      const Compare &comp = Compare{})
{
    static_assert(std::is_copy_constructible<Random_Access_Iterator>::value,
                  "Iterators must be copy constructible.");
    static_assert(std::is_copy_assignable<Random_Access_Iterator>::value,
                  "Iterators must be copy assignable.");
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    detail::bubble_up(first, last - Difference_Type(1), comp, detail::Flip<Compare>{ comp });
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
void pop_minmax_heap_min(Random_Access_Iterator first, Random_Access_Iterator last,
                         const Compare &comp = Compare{})
{
    static_assert(std::is_copy_constructible<Random_Access_Iterator>::value,
                  "Iterators must be copy constructible.");
    static_assert(std::is_copy_assignable<Random_Access_Iterator>::value,
                  "Iterators must be copy assignable.");

    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    Difference_Type n = last - first;
    if (n <= 1)
        return;
    detail::swap_iterator_values(first, last - Difference_Type(1));
    detail::trickle_down(first, n - 1, first, comp, detail::Flip<Compare>{ comp });
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
void pop_minmax_heap_max(Random_Access_Iterator first, Random_Access_Iterator last,
                         const Compare &comp = Compare{})
{
    static_assert(std::is_copy_constructible<Random_Access_Iterator>::value,
                  "Iterators must be copy constructible.");
    static_assert(std::is_copy_assignable<Random_Access_Iterator>::value,
                  "Iterators must be copy assignable.");
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    Difference_Type n = last - first;
    if (n < 2)
        return;
    auto pos = first + Difference_Type((n >= 3 && comp(*(first + 1), *(first + 2))) ? 2 : 1);
    auto new_last = last - Difference_Type(1);
    detail::swap_iterator_values(pos, new_last);
    update_minmax_heap(first, new_last, pos, comp);
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
Random_Access_Iterator is_minmax_heap_until(Random_Access_Iterator first,
                                            Random_Access_Iterator last,
                                            const Compare &comp = Compare{})
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    Difference_Type n = last - first;
    detail::Flip<Compare> flip_comp = { comp };

    if (1 < n && comp(*(first + Difference_Type(1)), *first)) {
        return first + Difference_Type(1); // left child of root violated reverse-priority order
    }
    if (2 < n && comp(*(first + Difference_Type(2)), *first)) {
        return first + Difference_Type(2); // right child of root violated reverse-priority order
    }

    for (Difference_Type i = 3; i < n; ++i) {
        auto it = first + Difference_Type(i);

        if (detail::in_odd_level(i)) {
            const auto &comp_node_level = flip_comp;
            auto other = first + Difference_Type(detail::index_parent(i));
            if (comp_node_level(*other, *it))
                return it; // child violated reverse-priority order
            other = first + Difference_Type(detail::index_parent(detail::index_parent(i)));
            if (comp_node_level(*it, *other))
                return it; // grandchild violated priority order
        } else {
            const auto &comp_node_level = comp;
            auto other = first + Difference_Type(detail::index_parent(i));
            if (comp_node_level(*other, *it))
                return it; // child violated reverse-priority order
            other = first + Difference_Type(detail::index_parent(detail::index_parent(i)));
            if (comp_node_level(*it, *other))
                return it; // grandchild violated priority order
        }
    }

    return last;
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
bool is_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                    const Compare &comp = Compare{})
{
    return is_minmax_heap_until(first, last, comp) == last;
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
Random_Access_Iterator min_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator,
                                       const Compare & = Compare{})
{
    return first;
}

template <typename Random_Access_Iterator, typename Compare = std::less<>>
Random_Access_Iterator max_minmax_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                                       const Compare &comp = Compare{})
{
    using Difference_Type = typename std::iterator_traits<Random_Access_Iterator>::difference_type;
    Difference_Type n = last - first;
    if (n < 2) {
        return first;
    } else if (n == 2) {
        return first + Difference_Type(1);
    } else {
        auto a = first + Difference_Type(1);
        auto b = first + Difference_Type(2);
        return comp(*a, *b) ? b : a;
    }
}
} // namespace qpmu
