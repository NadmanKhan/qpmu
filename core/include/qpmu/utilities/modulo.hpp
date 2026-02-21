#pragma once

#include <type_traits>

namespace qpmu {

// Assuming Lower_Bound <= x, y < Upper_Bound, return (x + y) wrapped within [Lower_Bound,
// Upper_Bound)
template <typename T>
constexpr T wrapping_add(T x, T y, T lower_bound, T upper_bound) noexcept
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
                  "T must be either an integral or floating point type");

    if constexpr (std::is_integral<T>::value) {
        // Make T signed so that we can handle negative sums correctly
        using Signed = std::make_signed_t<T>;
        // Use a wider type to avoid overflow during addition
        using Wide = std::intmax_t;
        const Signed lower = Signed(lower_bound);
        const Signed upper = Signed(upper_bound);
        const Signed range = upper - lower;
        Wide sum = Wide(x) + Wide(y);
        if (sum >= upper) {
            sum -= range;
        } else if (sum < lower) {
            sum += range;
        }
        return T(sum);

    } else {
        const T range = upper_bound - lower_bound;
        T sum = x + y;
        if (sum >= upper_bound) {
            sum -= range;
        } else if (sum < lower_bound) {
            sum += range;
        }
        return sum;
    }
}

template <typename T, T Modulus>
constexpr T modulo_next(T x) noexcept
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(Modulus > 0, "Modulus must be positive");
    return x == Modulus - 1 ? 0 : x + 1;
}

template <typename T, T Modulus>
constexpr T modulo_prev(T x) noexcept
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    static_assert(Modulus > 0, "Modulus must be positive");
    return x == 0 ? Modulus - 1 : x - 1;
}

} // namespace qpmu