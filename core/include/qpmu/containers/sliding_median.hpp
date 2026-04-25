#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace qpmu {

template <typename T, std::size_t Max_N = 31>
class Sliding_Median
{
public:
    explicit Sliding_Median(std::size_t n = 0) : _n(std::min(n, Max_N)) {}

    void push(T value)
    {
        _window[_curr] = value;
        _curr = (_curr + 1) % _n;
        if (_count < _n)
            ++_count;
    }

    T median() const
    {
        if (_count == 0)
            return T{};

        std::array<T, Max_N> tmp;
        std::copy_n(_window.begin(), _count, tmp.begin());
        auto mid = tmp.begin() + _count / 2;
        std::nth_element(tmp.begin(), mid, tmp.begin() + _count);
        return *mid;
    }

    bool enabled() const { return _n > 0; }

private:
    std::array<T, Max_N> _window = {};
    std::size_t _n = 0;
    std::size_t _curr = 0;
    std::size_t _count = 0;
};

} // namespace qpmu
