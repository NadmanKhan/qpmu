#pragma once

#include <cstddef>
#include <array>
#include <utility>

namespace qpmu {

// Linear interpolation:
//    (y - y0) / (x - x0) = (y1 - y0) / (x1 - x0)
// => (y - y0) * (x1 - x0) = (y1 - y0) * (x - x0)
// => y - y0 = (y1 - y0) * (x - x0) / (x1 - x0)
// => y = y0 + (y1 - y0) * (x - x0) / (x1 - x0)
template <typename X, typename Y>
constexpr Y linear_interpolation(const X &x0, const Y &y0, const X &x1, const Y &y1,
                                 const X &x) noexcept
{
    return (x1 == x0) ? y0 // avoid division by zero
                      : y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

enum Linear_Regression_Option {
    Slope_Only = 1,
    Intercept_Only = 2,
    Both_Slope_And_Intercept = 3,
};

template <typename T, Linear_Regression_Option Option>
struct Linear_Regression_Return_Type;

template <typename T>
struct Linear_Regression_Return_Type<T, Slope_Only>
{
    using type = T;
};

template <typename T>
struct Linear_Regression_Return_Type<T, Intercept_Only>
{
    using type = T;
};

template <typename T>
struct Linear_Regression_Return_Type<T, Both_Slope_And_Intercept>
{
    using type = std::pair<T, T>;
};

template <typename T, Linear_Regression_Option Option>
using Linear_Regression_Return_Type_t = typename Linear_Regression_Return_Type<T, Option>::type;

template <typename T, std::size_t N, Linear_Regression_Option Option = Both_Slope_And_Intercept>
inline Linear_Regression_Return_Type_t<T, Option>
linear_regression(const std::array<T, N> &xs, const std::array<T, N> &ys) noexcept
{
    if constexpr (N == 0)
        return {};

    T sum_x = 0;
    T sum_y = 0;
    T sum_xy = 0;
    T sum_xx = 0;

    // Vectorized summation
#pragma omp simd reduction(+ : sum_x, sum_y, sum_xy, sum_xx)
    for (std::size_t i = 0; i < N; ++i) {
        const T &x = xs[i];
        const T &y = ys[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }

    const T denom = T(N) * sum_xx - sum_x * sum_x;
    if (denom == 0)
        return {};

    if constexpr (Option == Slope_Only) {
        const T slope = (T(N) * sum_xy - sum_x * sum_y) / denom;
        return slope;

    } else if constexpr (Option == Intercept_Only) {
        const T slope = (T(N) * sum_xy - sum_x * sum_y) / denom;
        const T intercept = (sum_y - slope * sum_x) / T(N);
        return intercept;

    } else { // Both_Slope_And_Intercept
        const T slope = (T(N) * sum_xy - sum_x * sum_y) / denom;
        const T intercept = (sum_y - slope * sum_x) / T(N);
        return { slope, intercept };
    }
}

} // namespace qpmu
