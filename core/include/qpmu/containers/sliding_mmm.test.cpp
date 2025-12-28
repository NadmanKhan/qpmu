#include <gtest/gtest.h>

#include "sliding_mmm.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>
#include <vector>

namespace qpmu::testing {

class Sliding_MMM_Test : public ::testing::Test
{
protected:
    // Helper to compute expected min from a window
    template <typename T>
    static T expected_min(const std::vector<T> &window)
    {
        return *std::min_element(window.begin(), window.end());
    }

    // Helper to compute expected max from a window
    template <typename T>
    static T expected_max(const std::vector<T> &window)
    {
        return *std::max_element(window.begin(), window.end());
    }

    // Helper to compute expected median (lower median for even-sized windows)
    template <typename T>
    static T expected_median_first(const std::vector<T> &window)
    {
        std::vector<T> sorted = window;
        std::sort(sorted.begin(), sorted.end());
        return sorted[(sorted.size() - 1) / 2];
    }

    // Helper to compute expected median (upper median for even-sized windows)
    template <typename T>
    static T expected_median_second(const std::vector<T> &window)
    {
        std::vector<T> sorted = window;
        std::sort(sorted.begin(), sorted.end());
        if (sorted.size() % 2 == 1) {
            return sorted[(sorted.size() - 1) / 2];
        } else {
            return sorted[sorted.size() / 2];
        }
    }

    // Helper to test that MMM values match expected values for a given window
    template <typename T, std::size_t N>
    static void expect_mmm_correct(const Sliding_MMM<T, N> &mmm, const std::vector<T> &window,
                                   const std::string &context = "")
    {
        ASSERT_EQ(window.size(), N) << context;

        T exp_min = expected_min(window);
        T exp_max = expected_max(window);
        T exp_median_first = expected_median_first(window);
        T exp_median_second = expected_median_second(window);

        EXPECT_EQ(mmm.min(), exp_min) << context << " - min mismatch";
        EXPECT_EQ(mmm.max(), exp_max) << context << " - max mismatch";
        EXPECT_EQ(mmm.median_first(), exp_median_first) << context << " - median_first mismatch";
        EXPECT_EQ(mmm.median_second(), exp_median_second)
                << context << " - median_second mismatch";
    }
};

// ============================================================================
// Basic Construction and Single Window Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Construction_Size_1)
{
    Sliding_MMM<int, 1> mmm;
    mmm.push_next(42);
    EXPECT_EQ(mmm.min(), 42);
    EXPECT_EQ(mmm.max(), 42);
    EXPECT_EQ(mmm.median_first(), 42);
    EXPECT_EQ(mmm.median_second(), 42);
}

TEST_F(Sliding_MMM_Test, Construction_Size_2)
{
    Sliding_MMM<int, 2> mmm;
    mmm.push_next(10);
    mmm.push_next(20);

    expect_mmm_correct(mmm, { 10, 20 });
}

TEST_F(Sliding_MMM_Test, Construction_Size_3)
{
    Sliding_MMM<int, 3> mmm;
    mmm.push_next(10);
    mmm.push_next(30);
    mmm.push_next(20);

    expect_mmm_correct(mmm, { 10, 30, 20 });
}

TEST_F(Sliding_MMM_Test, Construction_Size_5)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 5, 2, 8, 1, 9 };
    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

// ============================================================================
// Sliding Window Behavior Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Sliding_Window_Size_3_Sequential)
{
    Sliding_MMM<int, 3> mmm;
    std::vector<int> stream = { 1, 2, 3, 4, 5, 6, 7 };
    std::vector<int> window;

    for (size_t i = 0; i < stream.size(); ++i) {
        mmm.push_next(stream[i]);

        // Build expected window
        window.push_back(stream[i]);
        if (window.size() > 3) {
            window.erase(window.begin());
        }

        if (window.size() == 3) {
            expect_mmm_correct(mmm, window, "Stream index " + std::to_string(i));
        }
    }
}

TEST_F(Sliding_MMM_Test, Sliding_Window_Size_5_Random)
{
    Sliding_MMM<int, 5> mmm;
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(1, 100);

    std::vector<int> window;

    for (int i = 0; i < 50; ++i) {
        int value = dist(gen);
        mmm.push_next(value);

        window.push_back(value);
        if (window.size() > 5) {
            window.erase(window.begin());
        }

        if (window.size() == 5) {
            expect_mmm_correct(mmm, window, "Iteration " + std::to_string(i));
        }
    }
}

TEST_F(Sliding_MMM_Test, Sliding_Window_Overwrite_Oldest)
{
    Sliding_MMM<int, 3> mmm;

    // Fill window: [1, 2, 3]
    mmm.push_next(1);
    mmm.push_next(2);
    mmm.push_next(3);
    expect_mmm_correct(mmm, { 1, 2, 3 });

    // Push 4, window becomes [2, 3, 4]
    mmm.push_next(4);
    expect_mmm_correct(mmm, { 2, 3, 4 });

    // Push 5, window becomes [3, 4, 5]
    mmm.push_next(5);
    expect_mmm_correct(mmm, { 3, 4, 5 });

    // Push 0, window becomes [4, 5, 0]
    mmm.push_next(0);
    expect_mmm_correct(mmm, { 4, 5, 0 });
}

// ============================================================================
// Min/Max Tracking Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Min_Tracking_Size_5)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 10, 5, 15, 3, 20 };

    for (int val : values) {
        mmm.push_next(val);
    }
    EXPECT_EQ(mmm.min(), 3);

    // Replace minimum with larger value
    mmm.push_next(8); // window: [5, 15, 3, 20, 8]
    EXPECT_EQ(mmm.min(), 3);

    mmm.push_next(12); // window: [15, 3, 20, 8, 12]
    EXPECT_EQ(mmm.min(), 3);

    mmm.push_next(7); // window: [3, 20, 8, 12, 7]
    EXPECT_EQ(mmm.min(), 3);

    mmm.push_next(4); // window: [20, 8, 12, 7, 4]
    EXPECT_EQ(mmm.min(), 4);
}

TEST_F(Sliding_MMM_Test, Max_Tracking_Size_5)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 10, 5, 15, 3, 20 };

    for (int val : values) {
        mmm.push_next(val);
    }
    EXPECT_EQ(mmm.max(), 20);

    // Replace maximum with smaller value
    mmm.push_next(8); // window: [5, 15, 3, 20, 8]
    EXPECT_EQ(mmm.max(), 20);

    mmm.push_next(12); // window: [15, 3, 20, 8, 12]
    EXPECT_EQ(mmm.max(), 20);

    mmm.push_next(7); // window: [3, 20, 8, 12, 7]
    EXPECT_EQ(mmm.max(), 20);

    mmm.push_next(4); // window: [20, 8, 12, 7, 4]
    EXPECT_EQ(mmm.max(), 20);

    mmm.push_next(2); // window: [8, 12, 7, 4, 2]
    EXPECT_EQ(mmm.max(), 12);
}

TEST_F(Sliding_MMM_Test, Min_Max_Alternating)
{
    Sliding_MMM<int, 4> mmm;

    // Alternating high-low pattern
    mmm.push_next(1);
    mmm.push_next(100);
    mmm.push_next(2);
    mmm.push_next(99);

    expect_mmm_correct(mmm, { 1, 100, 2, 99 });

    mmm.push_next(3); // window: [100, 2, 99, 3]
    expect_mmm_correct(mmm, { 100, 2, 99, 3 });

    mmm.push_next(98); // window: [2, 99, 3, 98]
    expect_mmm_correct(mmm, { 2, 99, 3, 98 });
}

// ============================================================================
// Median Tracking Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Median_Odd_Size_Window)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 5, 2, 8, 1, 9 };

    for (int val : values) {
        mmm.push_next(val);
    }

    // Sorted: [1, 2, 5, 8, 9], median = 5
    EXPECT_EQ(mmm.median_first(), 5);
    EXPECT_EQ(mmm.median_second(), 5); // Same for odd size
}

TEST_F(Sliding_MMM_Test, Median_Even_Size_Window)
{
    Sliding_MMM<int, 4> mmm;
    std::vector<int> values = { 5, 2, 8, 1 };

    for (int val : values) {
        mmm.push_next(val);
    }

    // Sorted: [1, 2, 5, 8]
    // median_first (lower) = 2, median_second (upper) = 5
    EXPECT_EQ(mmm.median_first(), 2);
    EXPECT_EQ(mmm.median_second(), 5);
}

TEST_F(Sliding_MMM_Test, Median_Updates_Correctly)
{
    Sliding_MMM<int, 3> mmm;

    mmm.push_next(5);
    mmm.push_next(10);
    mmm.push_next(15);
    // Sorted: [5, 10, 15], median = 10
    EXPECT_EQ(mmm.median_first(), 10);

    mmm.push_next(1); // window: [10, 15, 1], sorted: [1, 10, 15], median = 10
    EXPECT_EQ(mmm.median_first(), 10);

    mmm.push_next(20); // window: [15, 1, 20], sorted: [1, 15, 20], median = 15
    EXPECT_EQ(mmm.median_first(), 15);

    mmm.push_next(2); // window: [1, 20, 2], sorted: [1, 2, 20], median = 2
    EXPECT_EQ(mmm.median_first(), 2);
}

TEST_F(Sliding_MMM_Test, Median_Size_2)
{
    Sliding_MMM<int, 2> mmm;

    mmm.push_next(10);
    mmm.push_next(20);

    // Sorted: [10, 20]
    // median_first (lower) = 10, median_second (upper) = 20
    EXPECT_EQ(mmm.median_first(), 10);
    EXPECT_EQ(mmm.median_second(), 20);

    mmm.push_next(5); // window: [20, 5], sorted: [5, 20]
    EXPECT_EQ(mmm.median_first(), 5);
    EXPECT_EQ(mmm.median_second(), 20);
}

// ============================================================================
// Edge Cases and Special Values
// ============================================================================

TEST_F(Sliding_MMM_Test, All_Same_Values)
{
    Sliding_MMM<int, 5> mmm;

    for (int i = 0; i < 10; ++i) {
        mmm.push_next(42);
    }
    EXPECT_EQ(mmm.min(), 42);
    EXPECT_EQ(mmm.max(), 42);
    EXPECT_EQ(mmm.median_first(), 42);
    EXPECT_EQ(mmm.median_second(), 42);
}

TEST_F(Sliding_MMM_Test, Sorted_Ascending)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 1, 2, 3, 4, 5 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);

    // Continue sliding
    mmm.push_next(6); // window: [2, 3, 4, 5, 6]
    expect_mmm_correct(mmm, { 2, 3, 4, 5, 6 });
}

TEST_F(Sliding_MMM_Test, Sorted_Descending)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 10, 8, 6, 4, 2 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);

    // Continue sliding
    mmm.push_next(0); // window: [8, 6, 4, 2, 0]
    expect_mmm_correct(mmm, { 8, 6, 4, 2, 0 });
}

TEST_F(Sliding_MMM_Test, Negative_Values)
{
    Sliding_MMM<int, 4> mmm;
    std::vector<int> values = { -5, -2, -8, -1 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

TEST_F(Sliding_MMM_Test, Mixed_Positive_Negative)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { -10, 5, -3, 8, -1 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

TEST_F(Sliding_MMM_Test, Extreme_Values_Int)
{
    Sliding_MMM<int, 3> mmm;
    std::vector<int> values = { std::numeric_limits<int>::min(),
                                std::numeric_limits<int>::max(), 0 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

TEST_F(Sliding_MMM_Test, Double_Values)
{
    Sliding_MMM<double, 5> mmm;
    std::vector<double> values = { 1.1, 2.2, 3.3, 4.4, 5.5 };

    for (double val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);

    mmm.push_next(0.5);
    expect_mmm_correct(mmm, { 2.2, 3.3, 4.4, 5.5, 0.5 });
}

TEST_F(Sliding_MMM_Test, Duplicates)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> values = { 5, 5, 2, 2, 5 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);

    mmm.push_next(2); // window: [5, 2, 2, 5, 2]
    expect_mmm_correct(mmm, { 5, 2, 2, 5, 2 });
}

// ============================================================================
// Different Window Sizes
// ============================================================================

TEST_F(Sliding_MMM_Test, Window_Size_1)
{
    Sliding_MMM<int, 1> mmm;

    for (int val : { 10, 5, 15, 3, 20, 8 }) {
        mmm.push_next(val);
        EXPECT_EQ(mmm.min(), val);
        EXPECT_EQ(mmm.max(), val);
        EXPECT_EQ(mmm.median_first(), val);
        EXPECT_EQ(mmm.median_second(), val);
    }
}

TEST_F(Sliding_MMM_Test, Window_Size_7)
{
    Sliding_MMM<int, 7> mmm;
    std::vector<int> values = { 5, 2, 8, 1, 9, 3, 7 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

TEST_F(Sliding_MMM_Test, Window_Size_8)
{
    Sliding_MMM<int, 8> mmm;
    std::vector<int> values = { 5, 2, 8, 1, 9, 3, 7, 4 };

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

TEST_F(Sliding_MMM_Test, Window_Size_16)
{
    Sliding_MMM<int, 16> mmm;
    std::vector<int> values(16);
    std::iota(values.begin(), values.end(), 1);
    std::shuffle(values.begin(), values.end(), std::mt19937{ 123 });

    for (int val : values) {
        mmm.push_next(val);
    }

    expect_mmm_correct(mmm, values);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Stress_Large_Stream_Size_10)
{
    Sliding_MMM<int, 10> mmm;
    std::mt19937 gen(12345);
    std::uniform_int_distribution<int> dist(-1000, 1000);

    std::vector<int> window;

    for (int i = 0; i < 1000; ++i) {
        int value = dist(gen);
        mmm.push_next(value);

        window.push_back(value);
        if (window.size() > 10) {
            window.erase(window.begin());
        }

        if (window.size() == 10) {
            expect_mmm_correct(mmm, window);
        }
    }
}

TEST_F(Sliding_MMM_Test, Stress_Large_Stream_Size_50)
{
    Sliding_MMM<int, 50> mmm;
    std::mt19937 gen(54321);
    std::uniform_int_distribution<int> dist(-10000, 10000);

    std::vector<int> window;

    for (int i = 0; i < 500; ++i) {
        int value = dist(gen);
        mmm.push_next(value);

        window.push_back(value);
        if (window.size() > 50) {
            window.erase(window.begin());
        }

        if (window.size() == 50) {
            expect_mmm_correct(mmm, window);
        }
    }
}

TEST_F(Sliding_MMM_Test, Stress_Various_Window_Sizes)
{
    std::mt19937 gen(99999);
    std::uniform_int_distribution<int> dist(-100, 100);

    // Test various power-of-2 and power-of-2-minus-1 sizes
    auto test_size = [&](auto size_tag) {
        constexpr std::size_t N = decltype(size_tag)::value;
        Sliding_MMM<int, N> mmm;
        std::vector<int> window;

        for (int i = 0; i < 100; ++i) {
            int value = dist(gen);
            mmm.push_next(value);

            window.push_back(value);
            if (window.size() > N) {
                window.erase(window.begin());
            }

            if (window.size() == N) {
                expect_mmm_correct(mmm, window);
            }
        }
    };

    test_size(std::integral_constant<std::size_t, 1>{});
    test_size(std::integral_constant<std::size_t, 2>{});
    test_size(std::integral_constant<std::size_t, 3>{});
    test_size(std::integral_constant<std::size_t, 4>{});
    test_size(std::integral_constant<std::size_t, 7>{});
    test_size(std::integral_constant<std::size_t, 8>{});
    test_size(std::integral_constant<std::size_t, 15>{});
    test_size(std::integral_constant<std::size_t, 16>{});
    test_size(std::integral_constant<std::size_t, 31>{});
    test_size(std::integral_constant<std::size_t, 32>{});
}

// ============================================================================
// Monotonic Sequence Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Strictly_Increasing_Stream)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> window;

    for (int i = 0; i < 20; ++i) {
        mmm.push_next(i);

        window.push_back(i);
        if (window.size() > 5) {
            window.erase(window.begin());
        }

        if (window.size() == 5) {
            expect_mmm_correct(mmm, window, "Value " + std::to_string(i));
        }
    }
}

TEST_F(Sliding_MMM_Test, Strictly_Decreasing_Stream)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> window;

    for (int i = 20; i >= 0; --i) {
        mmm.push_next(i);

        window.push_back(i);
        if (window.size() > 5) {
            window.erase(window.begin());
        }

        if (window.size() == 5) {
            expect_mmm_correct(mmm, window, "Value " + std::to_string(i));
        }
    }
}

// ============================================================================
// Pattern-Based Tests
// ============================================================================

TEST_F(Sliding_MMM_Test, Sawtooth_Pattern)
{
    Sliding_MMM<int, 6> mmm;
    std::vector<int> window;

    // Sawtooth: 1, 2, 3, 1, 2, 3, 1, 2, 3, ...
    for (int i = 0; i < 30; ++i) {
        int value = (i % 3) + 1;
        mmm.push_next(value);

        window.push_back(value);
        if (window.size() > 6) {
            window.erase(window.begin());
        }

        if (window.size() == 6) {
            expect_mmm_correct(mmm, window, "Index " + std::to_string(i));
        }
    }
}

TEST_F(Sliding_MMM_Test, Square_Wave_Pattern)
{
    Sliding_MMM<int, 5> mmm;
    std::vector<int> window;

    // Square wave: 0, 0, 0, 100, 100, 100, 0, 0, 0, 100, ...
    for (int i = 0; i < 30; ++i) {
        int value = ((i / 3) % 2) * 100;
        mmm.push_next(value);

        window.push_back(value);
        if (window.size() > 5) {
            window.erase(window.begin());
        }

        if (window.size() == 5) {
            expect_mmm_correct(mmm, window, "Index " + std::to_string(i));
        }
    }
}

// ============================================================================
// Multiple Independent Windows
// ============================================================================

TEST_F(Sliding_MMM_Test, Multiple_Independent_Windows)
{
    Sliding_MMM<int, 3> mmm1;
    Sliding_MMM<int, 3> mmm2;
    Sliding_MMM<int, 3> mmm3;

    mmm1.push_next(1);
    mmm1.push_next(2);
    mmm1.push_next(3);

    mmm2.push_next(10);
    mmm2.push_next(20);
    mmm2.push_next(30);

    mmm3.push_next(100);
    mmm3.push_next(200);
    mmm3.push_next(300);

    EXPECT_EQ(mmm1.min(), 1);
    EXPECT_EQ(mmm1.max(), 3);

    EXPECT_EQ(mmm2.min(), 10);
    EXPECT_EQ(mmm2.max(), 30);

    EXPECT_EQ(mmm3.min(), 100);
    EXPECT_EQ(mmm3.max(), 300);
}

// ============================================================================
// Custom Type Tests
// ============================================================================

struct Point
{
    int x;
    int y;

    bool operator<(const Point &other) const
    {
        // Compare by Manhattan distance from origin
        int d1 = std::abs(x) + std::abs(y);
        int d2 = std::abs(other.x) + std::abs(other.y);
        return std::make_tuple(d1, x, y) < std::make_tuple(d2, other.x, other.y);
    }

    bool operator>(const Point &other) const { return other < *this; }

    bool operator==(const Point &other) const { return x == other.x && y == other.y; }
};

TEST_F(Sliding_MMM_Test, Custom_Struct_Type)
{
    Sliding_MMM<Point, 3> mmm;

    mmm.push_next({ 0, 1 }); // distance: 1
    mmm.push_next({ 2, 2 }); // distance: 4
    mmm.push_next({ 1, 0 }); // distance: 1

    // Min should be one with smallest distance
    Point p1{ 0, 1 };
    Point p2{ 1, 0 };
    EXPECT_TRUE(mmm.min() == p1 || mmm.min() == p2);

    // Max should be {2, 2}
    EXPECT_EQ(mmm.max(), (Point{ 2, 2 }));
}

// ============================================================================
// Verification of Internal Consistency
// ============================================================================

TEST_F(Sliding_MMM_Test, Min_Less_Than_Or_Equal_Median)
{
    Sliding_MMM<int, 7> mmm;
    std::mt19937 gen(777);
    std::uniform_int_distribution<int> dist(-100, 100);

    for (int i = 0; i < 100; ++i) {
        mmm.push_next(dist(gen));
        if (i >= 6) {
            EXPECT_LE(mmm.min(), mmm.median_first());
            EXPECT_LE(mmm.min(), mmm.median_second());
        }
    }
}

TEST_F(Sliding_MMM_Test, Median_Less_Than_Or_Equal_Max)
{
    Sliding_MMM<int, 7> mmm;
    std::mt19937 gen(888);
    std::uniform_int_distribution<int> dist(-100, 100);

    for (int i = 0; i < 100; ++i) {
        mmm.push_next(dist(gen));
        if (i >= 6) {
            EXPECT_LE(mmm.median_first(), mmm.max());
            EXPECT_LE(mmm.median_second(), mmm.max());
        }
    }
}

TEST_F(Sliding_MMM_Test, Median_First_Less_Than_Or_Equal_Median_Second)
{
    Sliding_MMM<int, 8> mmm; // Even size to have distinct medians
    std::mt19937 gen(999);
    std::uniform_int_distribution<int> dist(-100, 100);

    for (int i = 0; i < 100; ++i) {
        mmm.push_next(dist(gen));
        if (i >= 7) {
            EXPECT_LE(mmm.median_first(), mmm.median_second());
        }
    }
}

} // namespace qpmu::testing
