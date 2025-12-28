#pragma once

#include <cstddef>

#include "qpmu/algorithms/minmax_heap.hpp"
#include "qpmu/utilities/modulo.hpp"

namespace qpmu {

template <typename T>
struct MMM_Element
{
    T value;
    std::size_t *window_index;

    friend constexpr bool operator<(const MMM_Element<T> &a, const MMM_Element<T> &b)
    {
        return a.value < b.value;
    }
};

// Custom swap to update indices as well -- ADL will find this
template <typename T>
void iter_swap(MMM_Element<T> *a, MMM_Element<T> *b)
{
    std::swap(*(a->window_index), *(b->window_index));
    std::swap(*a, *b);
}

template <std::size_t N>
constexpr std::size_t Split = (N + 1) / 2;

// Sliding Min-Max-Median Heap for fixed-size window over streaming data
template <typename T, std::size_t N>
class Sliding_MMM
{
public:
    Sliding_MMM()
    {
        for (std::size_t i = 0; i < N; ++i) {
            _position_map[i] = i;
            _heap[i].window_index = &_position_map[i];
        }
    }

    void push_next(const T &value)
    {
        auto pos = _position_map[_curr];

        // Update element
        _heap[pos].value = value;
        _heap[pos].window_index = &_position_map[_curr];

        if (pos < Split<N>) {
            // In lower minmax heap
            update_minmax_heap(_heap, _heap + Split<N>, _heap + pos);
        } else {
            // In upper minmax heap
            update_minmax_heap(_heap + Split<N>, _heap + N, _heap + pos);
        }

        // Rebalance heaps to ensure "max of lower" <= "min of upper"
        if (N > 1) {
            auto max_lower = max_minmax_heap(_heap, _heap + Split<N>);
            auto min_upper = min_minmax_heap(_heap + Split<N>, _heap + N);
            if (min_upper->value < max_lower->value) {
                // Swap and update
                iter_swap(max_lower, min_upper);
                update_minmax_heap(_heap, _heap + Split<N>, max_lower);
                update_minmax_heap(_heap + Split<N>, _heap + N, min_upper);
            }
        }

        // Advance window
        _curr = wrapped_sum<std::size_t, 0, N>(_curr, 1);
    }

    inline const T &operator[](std::size_t index) const noexcept
    {
        return _heap[_position_map[index]].value;
    }

    inline const T &min() const noexcept { return _heap[0].value; }

    inline const T &max() const noexcept
    {
        if constexpr (N <= 1) {
            return _heap[0].value;
        }
        return max_minmax_heap(_heap + Split<N>, _heap + N)->value;
    }

    inline const T &median_first() const noexcept
    {
        return max_minmax_heap(_heap, _heap + Split<N>)->value;
    }

    inline const T &median_second() const noexcept
    {
        if constexpr (N % 2 == 1) {
            return median_first();
        }
        return min_minmax_heap(_heap + Split<N>, _heap + N)->value;
    }

private:
    MMM_Element<T> _heap[N] = {};
    std::size_t _position_map[N] = {}; // Maps window index to heap index
    std::size_t _curr = 0; // Next window position to write
};

} // namespace qpmu
