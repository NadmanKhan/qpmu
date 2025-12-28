#include <gtest/gtest.h>

#include "linear_algebra.hpp"

#include <array>
#include <cmath>

namespace qpmu::testing {

class Linear_Algebra_Test : public ::testing::Test
{
protected:
    // Helper to check if two floating point values are approximately equal
    template <typename T>
    static bool approx_equal(T a, T b, T epsilon = T(1e-6))
    {
        return std::abs(a - b) <= epsilon;
    }
};

// ============================================================================
// Linear Interpolation Tests
// ============================================================================

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Basic)
{
    // y = 2x: points (0, 0) and (10, 20), interpolate at x=5
    double result = linear_interpolation(0.0, 0.0, 10.0, 20.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 10.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_At_Start_Point)
{
    // Interpolate at the starting point
    double result = linear_interpolation(0.0, 5.0, 10.0, 15.0, 0.0);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_At_End_Point)
{
    // Interpolate at the ending point
    double result = linear_interpolation(0.0, 5.0, 10.0, 15.0, 10.0);
    EXPECT_DOUBLE_EQ(result, 15.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Midpoint)
{
    // Interpolate at the exact midpoint
    double result = linear_interpolation(0.0, 0.0, 10.0, 100.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 50.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Negative_Values)
{
    // Points (-10, -5) and (10, 5), interpolate at x=0
    double result = linear_interpolation(-10.0, -5.0, 10.0, 5.0, 0.0);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Negative_Slope)
{
    // Points (0, 10) and (10, 0), interpolate at x=5
    double result = linear_interpolation(0.0, 10.0, 10.0, 0.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Zero_Division_Protection)
{
    // When x0 == x1, should return y0 to avoid division by zero
    double result = linear_interpolation(5.0, 10.0, 5.0, 20.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 10.0);

    // Even when interpolating at a different x
    result = linear_interpolation(5.0, 10.0, 5.0, 20.0, 7.0);
    EXPECT_DOUBLE_EQ(result, 10.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Extrapolation_Before)
{
    // Extrapolate before the starting point
    double result = linear_interpolation(10.0, 20.0, 20.0, 30.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 15.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Extrapolation_After)
{
    // Extrapolate after the ending point
    double result = linear_interpolation(10.0, 20.0, 20.0, 30.0, 25.0);
    EXPECT_DOUBLE_EQ(result, 35.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Horizontal_Line)
{
    // Horizontal line: y = 5 for all x
    double result = linear_interpolation(0.0, 5.0, 10.0, 5.0, 3.0);
    EXPECT_DOUBLE_EQ(result, 5.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Integer_Types)
{
    // Test with integer types
    int result = linear_interpolation(0, 0, 10, 100, 5);
    EXPECT_EQ(result, 50);

    // Test integer division behavior
    result = linear_interpolation(0, 0, 10, 99, 5);
    EXPECT_EQ(result, 49); // Integer division truncates
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Float_Types)
{
    // Test with float types
    float result = linear_interpolation(0.0f, 0.0f, 10.0f, 20.0f, 5.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Mixed_Types)
{
    // X type is int, Y type is double
    double result = linear_interpolation(0, 0.0, 10, 100.0, 3);
    EXPECT_DOUBLE_EQ(result, 30.0);
}

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Constexpr)
{
    // Verify that the function is constexpr
    constexpr double result = linear_interpolation(0.0, 0.0, 10.0, 20.0, 5.0);
    EXPECT_DOUBLE_EQ(result, 10.0);
}

// ============================================================================
// Linear Regression Tests - Basic Functionality
// ============================================================================

TEST_F(Linear_Algebra_Test, Linear_Regression_Perfect_Fit_Through_Origin)
{
    // Perfect line: y = 2x
    std::array<double, 5> xs = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    std::array<double, 5> ys = { 0.0, 2.0, 4.0, 6.0, 8.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 2.0));
    EXPECT_TRUE(approx_equal(intercept, 0.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Perfect_Fit_With_Intercept)
{
    // Perfect line: y = 3x + 5
    std::array<double, 5> xs = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    std::array<double, 5> ys = { 5.0, 8.0, 11.0, 14.0, 17.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 3.0));
    EXPECT_TRUE(approx_equal(intercept, 5.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Negative_Slope)
{
    // Perfect line: y = -2x + 10
    std::array<double, 5> xs = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    std::array<double, 5> ys = { 10.0, 8.0, 6.0, 4.0, 2.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, -2.0));
    EXPECT_TRUE(approx_equal(intercept, 10.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Horizontal_Line)
{
    // Horizontal line: y = 5
    std::array<double, 5> xs = { 0.0, 1.0, 2.0, 3.0, 4.0 };
    std::array<double, 5> ys = { 5.0, 5.0, 5.0, 5.0, 5.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 0.0));
    EXPECT_TRUE(approx_equal(intercept, 5.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Two_Points)
{
    // Minimum case with 2 points: y = 4x - 1
    std::array<double, 2> xs = { 0.0, 1.0 };
    std::array<double, 2> ys = { -1.0, 3.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 4.0));
    EXPECT_TRUE(approx_equal(intercept, -1.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Single_Point)
{
    // Edge case: single point
    std::array<double, 1> xs = { 5.0 };
    std::array<double, 1> ys = { 10.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // With one point, we expect the result to be degenerate
    // The function returns (0, 0) when denom == 0
    EXPECT_DOUBLE_EQ(slope, 0.0);
    EXPECT_DOUBLE_EQ(intercept, 0.0);
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Vertical_Line_Protection)
{
    // All x values are the same (vertical line case)
    std::array<double, 5> xs = { 3.0, 3.0, 3.0, 3.0, 3.0 };
    std::array<double, 5> ys = { 1.0, 2.0, 3.0, 4.0, 5.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should return (0, 0) to avoid division by zero
    EXPECT_DOUBLE_EQ(slope, 0.0);
    EXPECT_DOUBLE_EQ(intercept, 0.0);
}

TEST_F(Linear_Algebra_Test, Linear_Regression_With_Noise)
{
    // Approximate line y = 2x + 1 with some noise
    std::array<double, 10> xs = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
    std::array<double, 10> ys = { 1.1, 2.9, 5.2, 6.8, 9.1, 10.9, 13.2, 14.8, 17.1, 18.9 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should be close to y = 2x + 1
    EXPECT_TRUE(approx_equal(slope, 2.0, 0.1));
    EXPECT_TRUE(approx_equal(intercept, 1.0, 0.5));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Negative_Values)
{
    // Line with negative values
    std::array<double, 5> xs = { -2.0, -1.0, 0.0, 1.0, 2.0 };
    std::array<double, 5> ys = { -7.0, -4.0, -1.0, 2.0, 5.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should be y = 3x - 1
    EXPECT_TRUE(approx_equal(slope, 3.0));
    EXPECT_TRUE(approx_equal(intercept, -1.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Non_Sequential_X)
{
    // X values not starting from 0 or 1
    std::array<double, 5> xs = { 10.0, 20.0, 30.0, 40.0, 50.0 };
    std::array<double, 5> ys = { 25.0, 45.0, 65.0, 85.0, 105.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should be y = 2x + 5
    EXPECT_TRUE(approx_equal(slope, 2.0));
    EXPECT_TRUE(approx_equal(intercept, 5.0));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Unordered_X)
{
    // X values not in order
    std::array<double, 5> xs = { 3.0, 1.0, 4.0, 0.0, 2.0 };
    std::array<double, 5> ys = { 11.0, 5.0, 14.0, 2.0, 8.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should still find y = 3x + 2
    EXPECT_TRUE(approx_equal(slope, 3.0));
    EXPECT_TRUE(approx_equal(intercept, 2.0));
}

// ============================================================================
// Linear Regression Tests - Different Data Types
// ============================================================================

TEST_F(Linear_Algebra_Test, Linear_Regression_Integer_Type)
{
    // Test with integer arrays
    std::array<int, 5> xs = { 0, 1, 2, 3, 4 };
    std::array<int, 5> ys = { 0, 2, 4, 6, 8 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_EQ(slope, 2);
    EXPECT_EQ(intercept, 0);
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Float_Type)
{
    // Test with float arrays
    std::array<float, 5> xs = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    std::array<float, 5> ys = { 1.0f, 3.0f, 5.0f, 7.0f, 9.0f };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_FLOAT_EQ(slope, 2.0f);
    EXPECT_FLOAT_EQ(intercept, 1.0f);
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Large_Array)
{
    // Test with larger array (100 points)
    std::array<double, 100> xs;
    std::array<double, 100> ys;

    // Generate y = 0.5x + 3
    for (std::size_t i = 0; i < 100; ++i) {
        xs[i] = static_cast<double>(i);
        ys[i] = 0.5 * xs[i] + 3.0;
    }

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 0.5));
    EXPECT_TRUE(approx_equal(intercept, 3.0));
}

// ============================================================================
// Linear Regression Tests - Edge Cases
// ============================================================================

TEST_F(Linear_Algebra_Test, Linear_Regression_Zero_Values)
{
    // All zeros
    std::array<double, 5> xs = { 0.0, 0.0, 0.0, 0.0, 0.0 };
    std::array<double, 5> ys = { 0.0, 0.0, 0.0, 0.0, 0.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should return (0, 0) for degenerate case
    EXPECT_DOUBLE_EQ(slope, 0.0);
    EXPECT_DOUBLE_EQ(intercept, 0.0);
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Extreme_Values)
{
    // Test with large values
    std::array<double, 5> xs = { 1e6, 2e6, 3e6, 4e6, 5e6 };
    std::array<double, 5> ys = { 2e6, 4e6, 6e6, 8e6, 10e6 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 2.0, 1e-5));
    EXPECT_TRUE(approx_equal(intercept, 0.0, 1e5));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Very_Small_Values)
{
    // Test with very small values
    std::array<double, 5> xs = { 1e-6, 2e-6, 3e-6, 4e-6, 5e-6 };
    std::array<double, 5> ys = { 2e-6, 4e-6, 6e-6, 8e-6, 10e-6 };

    auto [slope, intercept] = linear_regression(xs, ys);

    EXPECT_TRUE(approx_equal(slope, 2.0, 1e-5));
    EXPECT_TRUE(approx_equal(intercept, 0.0, 1e-11));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Duplicates)
{
    // Some duplicate points
    std::array<double, 6> xs = { 1.0, 1.0, 2.0, 2.0, 3.0, 3.0 };
    std::array<double, 6> ys = { 3.0, 3.0, 5.0, 5.0, 7.0, 7.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Should still find y = 2x + 1
    EXPECT_TRUE(approx_equal(slope, 2.0));
    EXPECT_TRUE(approx_equal(intercept, 1.0));
}

// ============================================================================
// Integration Tests - Using Regression Results with Interpolation
// ============================================================================

TEST_F(Linear_Algebra_Test, Regression_And_Interpolation_Integration)
{
    // Fit a line to some points, then use interpolation to predict
    std::array<double, 5> xs = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::array<double, 5> ys = { 2.5, 4.5, 6.5, 8.5, 10.5 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Use the regression line to predict at x = 3.5
    // Using two known points on the regression line
    double x0 = 3.0;
    double y0 = slope * x0 + intercept;
    double x1 = 4.0;
    double y1 = slope * x1 + intercept;

    double predicted = linear_interpolation(x0, y0, x1, y1, 3.5);

    // Direct calculation: y = 2x + 0.5, at x = 3.5: y = 7.5
    EXPECT_TRUE(approx_equal(predicted, 7.5));
}

TEST_F(Linear_Algebra_Test, Regression_Predicts_Training_Points)
{
    // Verify that the regression line passes through the mean point
    std::array<double, 5> xs = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::array<double, 5> ys = { 2.0, 4.0, 6.0, 8.0, 10.0 };

    auto [slope, intercept] = linear_regression(xs, ys);

    // Mean x = 3.0, mean y = 6.0
    double mean_x = 3.0;
    double predicted_y = slope * mean_x + intercept;

    EXPECT_TRUE(approx_equal(predicted_y, 6.0));
}

// ============================================================================
// Noexcept Tests
// ============================================================================

TEST_F(Linear_Algebra_Test, Linear_Interpolation_Is_Noexcept)
{
    // Verify that linear_interpolation is noexcept
    static_assert(noexcept(linear_interpolation(0.0, 0.0, 1.0, 1.0, 0.5)));
}

TEST_F(Linear_Algebra_Test, Linear_Regression_Is_Noexcept)
{
    // Verify that linear_regression is noexcept
    std::array<double, 5> xs = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::array<double, 5> ys = { 2.0, 4.0, 6.0, 8.0, 10.0 };
    static_assert(noexcept(linear_regression(xs, ys)));
}

// ============================================================================
// Real-World Use Case Tests
// ============================================================================

TEST_F(Linear_Algebra_Test, Temperature_Conversion_Use_Case)
{
    // Use linear interpolation for temperature conversion
    // Celsius to Fahrenheit: F = 9/5 * C + 32
    // Known points: (0°C, 32°F) and (100°C, 212°F)

    double celsius = 37.0; // Body temperature
    double fahrenheit = linear_interpolation(0.0, 32.0, 100.0, 212.0, celsius);

    EXPECT_TRUE(approx_equal(fahrenheit, 98.6));
}

TEST_F(Linear_Algebra_Test, Signal_Processing_Use_Case)
{
    // Simulate a sensor calibration scenario
    // Raw sensor readings vs actual values
    std::array<double, 10> raw_readings = { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 };
    std::array<double, 10> actual_values = { 10.5, 20.3, 30.1, 40.2, 50.0,
                                              59.8, 70.1, 79.9, 90.2, 100.1 };

    auto [slope, intercept] = linear_regression(raw_readings, actual_values);

    // Calibrate a new reading of 550
    double new_reading = 550.0;
    double calibrated = slope * new_reading + intercept;

    // Should be approximately 55.0
    EXPECT_TRUE(approx_equal(calibrated, 55.0, 1.0));
}

TEST_F(Linear_Algebra_Test, Time_Series_Trend_Use_Case)
{
    // Find trend in time series data
    std::array<double, 12> months = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    std::array<double, 12> sales = { 100, 110, 105, 120, 125, 130,
                                     135, 145, 140, 155, 160, 165 };

    auto [slope, intercept] = linear_regression(months, sales);

    // Positive slope indicates growing trend
    EXPECT_GT(slope, 0.0);

    // Predict sales for month 13
    double predicted_sales = slope * 13.0 + intercept;
    EXPECT_GT(predicted_sales, 165.0);
}

} // namespace qpmu::testing
