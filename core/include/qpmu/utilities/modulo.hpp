#pragma once

#include <cstddef>
#include <type_traits>

namespace qpmu {

// Assuming Lower_Bound <= x, y < Upper_Bound, return (x + y) wrapped within [Lower_Bound,
// Upper_Bound)
template <typename T, T Lower_Bound, T Upper_Bound>
constexpr T wrapped_sum(T x, T y) noexcept
{
    static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>,
                  "T must be either an integral or floating point type");
    static_assert(!std::is_same_v<T, bool>, "T must not be bool");
    static_assert(Lower_Bound < Upper_Bound, "Lower_Bound must be less than Upper_Bound");

    if constexpr (std::is_integral<T>::value) {
        // Make T signed so that we can handle negative sums correctly
        using Signed = std::make_signed_t<T>;
        // Use a wider type to avoid overflow during addition
        using Wide = std::intmax_t;
        constexpr Signed Lower = static_cast<Signed>(Lower_Bound);
        constexpr Signed Upper = static_cast<Signed>(Upper_Bound);
        constexpr Signed Range = Upper - Lower;
        Wide sum = static_cast<Wide>(x) + static_cast<Wide>(y);
        if (sum >= Upper) {
            sum -= Range;
        } else if (sum < Lower) {
            sum += Range;
        }
        return static_cast<T>(sum);

    } else {
        constexpr T Lower = static_cast<T>(Lower_Bound);
        constexpr T Upper = static_cast<T>(Upper_Bound);
        constexpr T Range = Upper - Lower;
        T sum = x + y;
        if (sum >= Upper) {
            sum -= Range;
        } else if (sum < Lower) {
            sum += Range;
        }
        return sum;
    }
}

} // namespace qpmu
