#include <gtest/gtest.h>

#include "minmax_heap.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace qpmu::testing {

class Minmax_Heap_Test : public ::testing::Test
{
protected:
    template <typename Random_Access_Iterator>
    static std::string stringify_heap(
            Random_Access_Iterator first, Random_Access_Iterator last,
            typename std::iterator_traits<Random_Access_Iterator>::difference_type mark_index = -1,
            std::string mark_message = {})
    {
        using Diffeerence_Type =
                typename std::iterator_traits<Random_Access_Iterator>::difference_type;

        std::stringstream ss;
        ss << "Heap: [";
        for (auto it = first; it != last; ++it) {
            if (it != first) {
                ss << ", ";
            }
            ss << *it;
        }
        ss << "]\n";
        Diffeerence_Type n = last - first;
        if (mark_index >= 0 && mark_index < n) {
            auto mark_it = first + mark_index;
            std::size_t offset_len = std::string("Heap: [").length();
            for (auto it = first; it != mark_it; ++it) {
                offset_len += (std::ostringstream() << *it << ", ").str().length();
            }
            std::string offset_str(offset_len, ' ');
            ss << offset_str << "^\n";
            for (char c : mark_message) {
                // if last character was newline, add offset
                if (ss.peek() == '\n') {
                    ss << offset_str << '|' << ' ';
                }
                ss << c;
            }
        }
        return ss.str();
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static Random_Access_Iterator is_valid_until(Random_Access_Iterator first,
                                                 Random_Access_Iterator last,
                                                 const Compare &comp = Compare{})
    {
        using Difference_Type =
                typename std::iterator_traits<Random_Access_Iterator>::difference_type;
        Difference_Type n = last - first;
        auto in_order = [&comp, first](Random_Access_Iterator above, Random_Access_Iterator below) {
            if (detail::in_odd_level(above - first)) {
                // max level => above >= below
                return !comp(*above, *below);
            } else {
                // min level => above <= below
                return !comp(*below, *above);
            }
        };
        for (Difference_Type i = 1; i < n; ++i) {
            auto it = first + Difference_Type(i);
            auto parent = first + Difference_Type(detail::index_parent(i));
            if (!in_order(parent, it)) {
                return it; // parent-child order violated
            }
            if (i >= 3) {
                auto grandparent =
                        first + Difference_Type(detail::index_parent(detail::index_parent(i)));
                if (!in_order(grandparent, it)) {
                    return it; // grandparent-child order violated
                }
            }
        }
        return last;
    }

    template <typename Iterator, typename Compare = std::less<>,
              void (*func)(Iterator, Iterator, const Compare &)>
    static void expect_preserves_content(Iterator first, Iterator last,
                                         const Compare &comp = Compare{})
    {
        using Value_Type = typename std::iterator_traits<Iterator>::value_type;

        std ::vector<Value_Type> before(first, last);
        std::sort(before.begin(), before.end(), comp);

        func(first, last, comp);

        std::vector<Value_Type> after(first, last);
        std::sort(after.begin(), after.end(), comp);

        EXPECT_EQ(before, after) << "Content mismatch before and after operation.\nBefore: "
                                 << stringify_heap(before.begin(), before.end())
                                 << "After: " << stringify_heap(after.begin(), after.end());
    }

    // Simply use the algorithm's own is_minmax_heap function for verification
    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void
    expect_valid_property(Random_Access_Iterator first, Random_Access_Iterator last,
                          const Compare &comp = {},
                          typename std::iterator_traits<Random_Access_Iterator>::difference_type
                                  expected_fault_index = -1)
    {
        using Diffeerence_Type =
                typename std::iterator_traits<Random_Access_Iterator>::difference_type;
        Diffeerence_Type len = last - first;
        if (expected_fault_index < 0 || expected_fault_index >= len) {
            expected_fault_index = last - first;
        }

        auto fault_it = is_valid_until(first, last, comp);
        auto expected_fault_it = first + expected_fault_index;

        if (fault_it != expected_fault_it) {
            Diffeerence_Type fault_index = fault_it - first;
            Diffeerence_Type mark_index = -1;
            std::ostringstream msg;
            if (expected_fault_it != last) {
                mark_index = expected_fault_index;
                msg << "Expected fault at index " << expected_fault_index << ".\n";
                if (fault_it == last) {
                    msg << "But the entire range is a valid min-max heap.\n";
                } else {
                    msg << "But fault found at index " << fault_index << ".\n";
                }
            }
            if (fault_it != last) {
                msg << "Min-max heap property violated at [" << fault_index << "] = " << *fault_it
                    << ". The property expects the value to be:\n";
                if (fault_index > 2) {
                    msg << " * " << "<>"[!detail::in_odd_level(fault_index)] << "="
                        << *(first + detail::index_parent(detail::index_parent(fault_index)))
                        << " (grandparent)\n";
                }
                msg << " * " << "<>"[detail::in_odd_level(fault_index)] << "="
                    << *(first + detail::index_parent(fault_index)) << " (parent)\n";
            }
            EXPECT_EQ(fault_it - first, expected_fault_it - first)
                    << stringify_heap(first, last, mark_index, msg.str());
        } else {
            EXPECT_EQ(fault_it - first, expected_fault_it - first);
        }
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_make(Random_Access_Iterator first, Random_Access_Iterator last,
                          const Compare &comp = Compare{})
    {
        expect_preserves_content<Random_Access_Iterator, Compare, make_minmax_heap>(first, last,
                                                                                    comp);
        expect_valid_property(first, last, comp);
        if (first != last) {
            EXPECT_EQ(*std::min_element(first, last, comp), *min_minmax_heap(first, last, comp));
            EXPECT_EQ(*std::max_element(first, last, comp), *max_minmax_heap(first, last, comp));
        }
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_update(Random_Access_Iterator first, Random_Access_Iterator last,
                            Random_Access_Iterator pos,
                            typename std::iterator_traits<Random_Access_Iterator>::value_type x,
                            const Compare &comp = Compare{})
    {
        *pos = x;
        update_minmax_heap(first, last, pos, comp);
        expect_valid_property(first, last, comp);
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_push(Random_Access_Iterator first, Random_Access_Iterator last,
                          typename std::iterator_traits<Random_Access_Iterator>::value_type x,
                          const Compare &comp = Compare{})
    {
        *(last++) = x; // assume space is available
        expect_preserves_content<Random_Access_Iterator, Compare, push_minmax_heap>(first, last,
                                                                                    comp);
        expect_valid_property(first, last, comp);
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_pop_min(
            Random_Access_Iterator first, Random_Access_Iterator last,
            typename std::iterator_traits<Random_Access_Iterator>::value_type *out_popped = nullptr,
            const Compare &comp = Compare{})
    {
        if (first == last) {
            return;
        }
        expect_preserves_content<Random_Access_Iterator, Compare, pop_minmax_heap_min>(first, last,
                                                                                       comp);
        EXPECT_EQ(*(std::min_element(first, last, comp)), *(last - 1));
        expect_valid_property(first, last - 1, comp);
        if (out_popped) {
            *out_popped = *(last - 1);
        }
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_pop_max(
            Random_Access_Iterator first, Random_Access_Iterator last,
            typename std::iterator_traits<Random_Access_Iterator>::value_type *out_popped = nullptr,
            const Compare &comp = Compare{})
    {
        if (first == last) {
            return;
        }
        expect_preserves_content<Random_Access_Iterator, Compare, pop_minmax_heap_max>(first, last,
                                                                                       comp);
        EXPECT_EQ(*(std::max_element(first, last, comp)), *(last - 1));
        expect_valid_property(first, last - 1, comp);
        if (out_popped) {
            *out_popped = *(last - 1);
        }
    }

    template <typename Random_Access_Iterator, typename Compare = std::less<>>
    static void test_is_heap(Random_Access_Iterator first, Random_Access_Iterator last,
                             const Compare &comp = Compare{})
    {
        auto fault_it = is_minmax_heap_until(first, last, comp);
        auto expected_fault_it = is_valid_until(first, last, comp);
        EXPECT_EQ(fault_it, expected_fault_it)
                << "Mismatch in is_minmax_heap_until result.\nExpected fault at index "
                << (expected_fault_it - first) << ", but got fault at index " << (fault_it - first)
                << ".\n"
                << stringify_heap(first, last);
        EXPECT_TRUE(is_minmax_heap(first, fault_it, comp));
    }

    template <typename T, typename Compare = std::less<>>
    static void test_vector_push(std::vector<T> &v, const T &x, const Compare &comp = Compare{},
                                 bool reverse = false)
    {
        if (reverse) {
            v.insert(v.begin(), x);
            test_push(v.rbegin(), v.rend() - 1, x, comp);
        } else {
            v.push_back(x);
            test_push(v.begin(), v.end() - 1, x, comp);
        }
    }

    template <typename T, typename Compare = std::less<>>
    static void test_vector_pop_min(std::vector<T> &v, T *out_popped = nullptr,
                                    const Compare &comp = Compare{}, bool reverse = false)
    {
        if (reverse) {
            test_pop_min(v.rbegin(), v.rend(), out_popped, comp);
            if (!v.empty()) {
                v.erase(v.begin());
            }
        } else {
            test_pop_min(v.begin(), v.end(), out_popped, comp);
            if (!v.empty()) {
                v.pop_back();
            }
        }
    }

    template <typename T, typename Compare = std::less<>>
    static void test_vector_pop_max(std::vector<T> &v, T *out_popped = nullptr,
                                    const Compare &comp = Compare{}, bool reverse = false)
    {
        if (reverse) {
            test_pop_max(v.rbegin(), v.rend(), out_popped, comp);
            if (!v.empty()) {
                v.erase(v.begin());
            }
        } else {
            test_pop_max(v.begin(), v.end(), out_popped, comp);
            if (!v.empty()) {
                v.pop_back();
            }
        }
    }
};

// ============================================================================
// Basic Heap Construction Tests (make_minmax_heap)
// ============================================================================

TEST_F(Minmax_Heap_Test, Make_Empty_Heap)
{
    std::vector<int> v;
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Single_Element)
{
    std::vector<int> v = { 42 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Two_Elements)
{
    // Ascending
    std::vector<int> v1 = { 1, 2 };
    test_make(v1.begin(), v1.end());

    // Descending
    std::vector<int> v2 = { 2, 1 };
    test_make(v2.begin(), v2.end());
}

TEST_F(Minmax_Heap_Test, Make_Three_Elements)
{
    std::vector<int> v = { 3, 1, 2 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Sorted_Ascending)
{
    std::vector<int> v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Sorted_Descending)
{
    std::vector<int> v = { 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Unsorted)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_With_Duplicates)
{
    std::vector<int> v = { 5, 2, 5, 1, 2, 5, 1, 2 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_All_Same)
{
    std::vector<int> v(10, 42);
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Negative_Values)
{
    std::vector<int> v = { -5, -2, -8, -1, -9, -3, -7, -4, -6 };
    test_make(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Make_Large_Heap)
{
    std::vector<int> v(1000);
    for (int i = 0; i < 1000; ++i) {
        v[i] = 1000 - i;
    }
    test_make(v.begin(), v.end());
}

// ============================================================================
// Heap Insertion Tests (push_minmax_heap)
// ============================================================================

TEST_F(Minmax_Heap_Test, Push_Single_Element)
{
    std::vector<int> v = { 42 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 10);
}

TEST_F(Minmax_Heap_Test, Push_New_Minimum)
{
    std::vector<int> v = { 5, 10, 7 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 1);
}

TEST_F(Minmax_Heap_Test, Push_New_Maximum)
{
    std::vector<int> v = { 1, 5, 3 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 100);
}

TEST_F(Minmax_Heap_Test, Push_Middle_Value)
{
    std::vector<int> v = { 1, 10, 5 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 6);
}

TEST_F(Minmax_Heap_Test, Push_Duplicate)
{
    std::vector<int> v = { 1, 5, 3 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 5);
}

TEST_F(Minmax_Heap_Test, Push_Multiple_Elements)
{
    std::vector<int> v = { 5 };
    test_make(v.begin(), v.end());
    std::vector<int> to_insert = { 3, 7, 1, 9, 2, 8, 4, 6 };
    for (int val : to_insert) {
        test_vector_push(v, val);
    }
}

// ============================================================================
// Heap Removal Tests (pop_minmax_heap_min, pop_minmax_heap_max)
// ============================================================================

TEST_F(Minmax_Heap_Test, Pop_Min_Small_Heaps)
{
    std::vector<int> v;

    // Test single element
    v = { 42 };
    test_vector_pop_min(v);

    // Test two elements
    v = { 1, 2 };
    test_vector_pop_min(v);

    // Test three elements
    v = { 1, 3, 2 };
    test_vector_pop_min(v);
}

TEST_F(Minmax_Heap_Test, Pop_Min_Multiple_Elements)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    test_make(v.begin(), v.end());

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_min(v, &out);
        popped.push_back(out);
    }

    // Elements should come out in sorted order (since we always pop min)
    EXPECT_TRUE(std::is_sorted(popped.begin(), popped.end()));
}

TEST_F(Minmax_Heap_Test, Pop_Min_All_From_Large_Heap)
{
    std::vector<int> v(100);
    for (int i = 0; i < 100; ++i) {
        v[i] = 100 - i;
    }
    test_make(v.begin(), v.end());

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_min(v, &out);
        popped.push_back(out);
    }

    EXPECT_TRUE(std::is_sorted(popped.begin(), popped.end()));
}

TEST_F(Minmax_Heap_Test, Pop_Max_Small_Heaps)
{
    std::vector<int> v;

    // Test single element
    v = { 42 };
    test_vector_pop_max(v);

    // Test two elements
    v = { 1, 2 };
    test_vector_pop_max(v);

    // Test three elements
    v = { 1, 3, 2 };
    test_vector_pop_max(v);
}

TEST_F(Minmax_Heap_Test, Pop_Max_Multiple_Elements)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    test_make(v.begin(), v.end());

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_max(v, &out);
        popped.push_back(out);
    }

    // Elements should come out in reverse sorted order (since we always pop max)
    EXPECT_TRUE(std::is_sorted(popped.rbegin(), popped.rend()));
}

TEST_F(Minmax_Heap_Test, Pop_Max_All_From_Large_Heap)
{
    std::vector<int> v(100);
    for (int i = 0; i < 100; ++i) {
        v[i] = i + 1;
    }
    test_make(v.begin(), v.end());

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_max(v, &out);
        popped.push_back(out);
    }

    EXPECT_TRUE(std::is_sorted(popped.rbegin(), popped.rend()));
}

TEST_F(Minmax_Heap_Test, Pop_Max_With_Duplicates)
{
    std::vector<int> v = { 5, 10, 10, 3, 8, 2, 10, 1 };
    test_make(v.begin(), v.end());

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_max(v, &out);
        popped.push_back(out);
    }

    EXPECT_TRUE(std::is_sorted(popped.rbegin(), popped.rend()));
}

TEST_F(Minmax_Heap_Test, Pop_Max_Alternating_With_Pop_Min)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6, 10 };
    test_make(v.begin(), v.end());

    std::vector<int> mins_popped;
    std::vector<int> maxs_popped;

    bool pop_min = true;
    while (!v.empty()) {
        int out;
        if (pop_min) {
            test_vector_pop_min(v, &out);
            mins_popped.push_back(out);
        } else {
            test_vector_pop_max(v, &out);
            maxs_popped.push_back(out);
        }
        pop_min = !pop_min;
    }

    EXPECT_TRUE(std::is_sorted(mins_popped.begin(), mins_popped.end()));
    EXPECT_TRUE(std::is_sorted(maxs_popped.rbegin(), maxs_popped.rend()));
}

TEST_F(Minmax_Heap_Test, Pop_Max_With_Custom_Comparator)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7 };
    test_make(v.begin(), v.end(), std::greater<>());

    std::vector<int> popped;
    while (!v.empty()) {
        // With greater comparator, "max" means smallest value
        int out;
        test_vector_pop_max(v, &out, std::greater<>());
        popped.push_back(out);
    }

    // With greater comparator, popping max gives us smallest values in ascending order
    EXPECT_TRUE(std::is_sorted(popped.begin(), popped.end()));
}

// ============================================================================
// Heap Update Tests (update_minmax_heap)
// ============================================================================

TEST_F(Minmax_Heap_Test, Decrease_Root)
{
    std::vector<int> v = { 5, 10, 8, 15, 12 };
    test_make(v.begin(), v.end());
    test_update(v.begin(), v.end(), v.begin(), 2);
}

TEST_F(Minmax_Heap_Test, Increase_Root)
{
    std::vector<int> v = { 5, 10, 8, 15, 12 };
    test_make(v.begin(), v.end());
    test_update(v.begin(), v.end(), v.begin(), 20);
}

TEST_F(Minmax_Heap_Test, Decrease_Middle_Position)
{
    std::vector<int> v = { 1, 10, 8, 15, 12, 9, 7 };
    test_make(v.begin(), v.end());
    test_update(v.begin(), v.end(), v.begin() + 3, 2);
}

TEST_F(Minmax_Heap_Test, Increase_Middle_Position)
{
    std::vector<int> v = { 1, 10, 8, 15, 12, 9, 7 };
    test_make(v.begin(), v.end());
    test_update(v.begin(), v.end(), v.begin() + 3, 20);
}

TEST_F(Minmax_Heap_Test, Random_Updates_At_Random_Positions)
{
    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    test_make(v.begin(), v.end());

    std::mt19937 gen(42);
    std::uniform_int_distribution<size_t> pos_dist(0, v.size() - 1);
    std::uniform_int_distribution<int> val_dist(-100, 100);

    for (int i = 0; i < 100; ++i) {
        size_t pos = pos_dist(gen);
        int new_val = val_dist(gen);
        test_update(v.begin(), v.end(), v.begin() + pos, new_val);
    }
}

// ============================================================================
// Heap Validation Tests (is_minmax_heap, is_minmax_heap_until)
// ============================================================================

TEST_F(Minmax_Heap_Test, Is_Heap_Empty)
{
    std::vector<int> v;
    test_is_heap(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Is_Heap_Single_Element)
{
    std::vector<int> v = { 42 };
    test_is_heap(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Is_Heap_Two_And_Three_Elements)
{
    std::vector<int> v = { 42 };
    test_is_heap(v.begin(), v.end());
    v = { 1, 2 };
    test_is_heap(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Is_Heap_Several_Random_Elements)
{
    std::vector<int> v;

    std::mt19937 gen(123);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    for (int i = 0; i < 50; ++i) {
        v.push_back(dist(gen));
    }
    make_minmax_heap(v.begin(), v.end());
    test_is_heap(v.begin(), v.end());
}

TEST_F(Minmax_Heap_Test, Is_Heap_Permuted_Elements)
{
    std::vector<int> v(9); // 9! permutations
    std::iota(v.begin(), v.end(), 1);
    do {
        test_is_heap(v.begin(), v.end());
    } while (std::next_permutation(v.begin(), v.end()));
}

// ============================================================================
// Floating Point and Special Value Tests
// ============================================================================

TEST_F(Minmax_Heap_Test, Double_Values)
{
    std::vector<double> v = { 5.5, 2.2, 8.8, 1.1, 9.9, 3.3, 7.7, 4.4, 6.6 };
    test_make(v.begin(), v.end());
    test_vector_push(v, 0.5);
}

TEST_F(Minmax_Heap_Test, Extreme_Values)
{
    std::vector<int> v = { std::numeric_limits<int>::max(), std::numeric_limits<int>::min(), 0,
                           std::numeric_limits<int>::max() / 2,
                           std::numeric_limits<int>::min() / 2 };

    test_make(v.begin(), v.end());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(Minmax_Heap_Test, Random_Large_Heap)
{
    std::mt19937 gen(12345);
    std::uniform_int_distribution<int> dist(-1000, 1000);

    std::vector<int> v;
    for (int i = 0; i < 1000; ++i) {
        test_vector_push(v, dist(gen));
    }

    std::vector<int> popped;
    while (!v.empty()) {
        int out;
        test_vector_pop_min(v, &out);
        popped.push_back(out);
    }

    EXPECT_TRUE(std::is_sorted(popped.begin(), popped.end()));
}

TEST_F(Minmax_Heap_Test, Push_Pop_Sequence)
{
    std::vector<int> v;
    std::mt19937 gen(54321);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    std::uniform_int_distribution<int> op_dist(0, 1);

    for (int i = 0; i < 1000; ++i) {
        if (v.empty() || op_dist(gen) == 0) {
            // Push
            test_vector_push(v, dist(gen));
        } else {
            // Pop
            test_vector_pop_min(v);
        }
    }
}

TEST_F(Minmax_Heap_Test, Various_Heap_Sizes)
{
    // Test powers of 2 and powers of 2 minus 1
    for (size_t size : { 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 255, 256, 511, 512 }) {
        std::vector<int> v(size);
        for (size_t i = 0; i < size; ++i) {
            v[i] = static_cast<int>(size - i);
        }
        test_make(v.begin(), v.end());
    }
}

// ============================================================================
// Array Container Tests (C-style and std::array)
// ============================================================================

TEST_F(Minmax_Heap_Test, C_Style_Array)
{
    int arr[] = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    constexpr size_t arr_size = sizeof(arr) / sizeof(arr[0]);
    test_make(arr, arr + arr_size);
}

TEST_F(Minmax_Heap_Test, C_Style_Array_Push_Pop)
{
    int arr[10] = { 5, 2, 8, 1, 9, 3, 7 };
    int *end = arr + 7;
    test_make(arr, end);
    test_push(arr, end++, 4);
    test_pop_min(arr, end--);
}

// ============================================================================
// Custom/Non-default Type, Comparator, Iterator Tests
// ============================================================================

// Custom struct with operator< defined
struct Point_With_Operator
{
    int x;
    int y;

    bool operator<(const Point_With_Operator &other) const
    {
        // Compare by distance from origin
        auto d1 = (x * x + y * y);
        auto d2 = (other.x * other.x + other.y * other.y);
        return std::make_tuple(d1, x, y) < std::make_tuple(d2, other.x, other.y);
    }

    bool operator==(const Point_With_Operator &other) const { return x == other.x && y == other.y; }

    friend std::ostream &operator<<(std::ostream &os, const Point_With_Operator &p)
    {
        return os << "(" << p.x << "," << p.y << ")";
    }
};

TEST_F(Minmax_Heap_Test, Custom_Struct_With_Operator)
{
    std::vector<Point_With_Operator> points = {
        { 0, 1 }, // distance: 1
        { 1, 0 }, // distance: 1
        { 1, 1 }, // distance: sqrt(2)
        { 0, 2 }, // distance: 2
        { 2, 0 }, // distance: 2
        { 2, 2 }, // distance: sqrt(8)
        { 3, 4 }, // distance: 5
        { 1, 1 }, // distance: sqrt(2)
        { 5, 12 }, // distance: 13
    };

    test_make(points.begin(), points.end());

    // Push a new minimum
    test_vector_push(points, { 0, 0 }); // distance: 0

    // Pop and verify sorted order
    std::vector<int> distances;
    while (!points.empty()) {
        Point_With_Operator out;
        test_vector_pop_min(points, &out);
        distances.push_back(out.x * out.x + out.y * out.y);
    }

    EXPECT_TRUE(std::is_sorted(distances.begin(), distances.end()));
}

// Custom struct for functor tests
struct Person
{
    std::string name;
    int age;
    double salary;

    bool operator==(const Person &other) const
    {
        return name == other.name && age == other.age && salary == other.salary;
    }

    friend std::ostream &operator<<(std::ostream &os, const Person &p)
    {
        return os << "{" << p.name << "," << p.age << ",$" << p.salary << "}";
    }
};

// Stateless functor
struct Compare_By_Age
{
    bool operator()(const Person &a, const Person &b) const { return a.age < b.age; }
};

// Stateful functor
struct Compare_By_Net_Salary
{
    struct Tax_Bracket
    {
        double threshold;
        double rate;
    };

    std::vector<Tax_Bracket> tax_brackets;

    explicit Compare_By_Net_Salary(std::vector<Tax_Bracket> brackets) : tax_brackets(brackets)
    {
        std::sort(tax_brackets.begin(), tax_brackets.end(),
                  [](const Tax_Bracket &a, const Tax_Bracket &b) {
                      return a.threshold > b.threshold
                              || (a.threshold == b.threshold && a.rate > b.rate);
                  });
    }

    bool operator()(const Person &a, const Person &b) const
    {
        double net_a = a.salary - calculate_tax(a.salary);
        double net_b = b.salary - calculate_tax(b.salary);
        return net_a < net_b;
    }

private:
    double calculate_tax(double salary) const
    {
        double tax = 0.0;
        for (const auto &bracket : tax_brackets) {
            if (salary > bracket.threshold) {
                tax += (salary - bracket.threshold) * bracket.rate;
                salary = bracket.threshold;
            }
        }
        return tax;
    }
};

TEST_F(Minmax_Heap_Test, Custom_Struct_Stateless_Functor)
{
    std::vector<Person> people = { { "Alice", 30, 500000.0 },
                                   { "Bob", 25, 60000.0 },
                                   { "Charlie", 35, 55000.0 },
                                   { "David", 28, 58000.0 },
                                   { "Eve", 22, 5200.0 } };

    test_make(people.begin(), people.end(), Compare_By_Age());

    // Min should be Eve
    EXPECT_EQ(people[0].name, "Eve");
}

TEST_F(Minmax_Heap_Test, Custom_Struct_Stateful_Functor)
{
    std::vector<Person> people = { { "Alice", 30, 500000.0 },
                                   { "Bob", 25, 60000.0 },
                                   { "Charlie", 35, 55000.0 },
                                   { "David", 28, 58000.0 },
                                   { "Eve", 22, 52000.0 } };

    std::vector<Compare_By_Net_Salary::Tax_Bracket> brackets = { { 100000.0, 10.0 },
                                                                 { 50000.0, 0.40 },
                                                                 { 10000.0, 0.10 } };
    Compare_By_Net_Salary comparator(brackets);

    test_make(people.begin(), people.end(), comparator);

    // Min should be Alice (highest salary, but stupid tax rates)
    EXPECT_EQ(people[0].name, "Alice");
}

TEST_F(Minmax_Heap_Test, Greater_Comparator)
{
    std::vector<int> v;
    std::mt19937 gen(777);
    std::uniform_int_distribution<int> value_dist(-100, 100);
    std::uniform_int_distribution<int> index_dist(0, 100);
    std::uniform_int_distribution<int> op_dist(0, 2);

    // Build initial heap
    for (int i = 0; i < 20; ++i) {
        v.push_back(value_dist(gen));
    }
    test_make(v.begin(), v.end());

    // Random push operations
    for (int i = 0; i < 1000; ++i) {
        test_vector_push(v, value_dist(gen));
    }

    // Random pop min/max operations
    for (int i = 0; i < 100; ++i) {
        auto op = op_dist(gen);
        if (op == 0) {
            // Pop min
            test_vector_pop_min<int>(v);
        } else {
            // Pop max
            test_vector_pop_max<int>(v);
        }
    }

    // Update random positions
    for (int i = 0; i < 100; ++i) {
        if (v.empty()) {
            break;
        }
        size_t index = index_dist(gen) % v.size();
        int new_value = value_dist(gen);
        test_update(v.begin(), v.end(), v.begin() + index, new_value);
    }
}

TEST_F(Minmax_Heap_Test, Reverse_Iterator)
{
    std::vector<int> v;
    std::mt19937 gen(777);
    std::uniform_int_distribution<int> value_dist(-100, 100);
    std::uniform_int_distribution<int> index_dist(0, 100);
    std::uniform_int_distribution<int> op_dist(0, 2);

    // Build initial heap
    for (int i = 0; i < 20; ++i) {
        v.push_back(value_dist(gen));
    }
    test_make(v.rbegin(), v.rend());

    // Random push operations
    for (int i = 0; i < 1000; ++i) {
        test_vector_push(v, value_dist(gen), {}, true);
    }

    // Random pop min/max operations
    for (int i = 0; i < 100; ++i) {
        auto op = op_dist(gen);
        if (op == 0) {
            // Pop min
            test_vector_pop_min<int>(v, nullptr, {}, true);
        } else {
            // Pop max
            test_vector_pop_max<int>(v, nullptr, {}, true);
        }
    }

    // Update random positions
    for (int i = 0; i < 100; ++i) {
        if (v.empty()) {
            break;
        }
        size_t index = index_dist(gen) % v.size();
        int new_value = value_dist(gen);
        test_update(v.rbegin(), v.rend(), v.rbegin() + index, new_value);
    }
}

// ============================================================================
// ADL (Argument-Dependent Lookup) Tests for iter_swap
// These tests verify that custom iter_swap implementations are properly
// found via ADL, enabling position tracking and other custom behaviors
// ============================================================================

namespace adl_test {

// Global swap log for tracking iter_swap calls
inline std::vector<std::pair<int, int>> *g_swap_log = nullptr;

// Custom type defined in adl_test namespace
// This enables ADL to find iter_swap defined in this namespace
struct Tracked_Int
{
    int value;

    Tracked_Int() : value(0) { }
    Tracked_Int(int v) : value(v) { }

    bool operator<(const Tracked_Int &other) const { return value < other.value; }
    bool operator>(const Tracked_Int &other) const { return value > other.value; }
    bool operator==(const Tracked_Int &other) const { return value == other.value; }

    friend std::ostream &operator<<(std::ostream &os, const Tracked_Int &ti)
    {
        return os << ti.value;
    }
};

// Helper to track base iterator
inline std::vector<Tracked_Int>::iterator g_tracked_int_base_iter;

// Specialized iter_swap for std::vector<Tracked_Int>::iterator
// ADL will find this because Tracked_Int is defined in adl_test namespace
inline void iter_swap(std::vector<Tracked_Int>::iterator a, std::vector<Tracked_Int>::iterator b)
{
    if (g_swap_log) {
        int pos_a = static_cast<int>(a - g_tracked_int_base_iter);
        int pos_b = static_cast<int>(b - g_tracked_int_base_iter);
        g_swap_log->push_back({ pos_a, pos_b });
    }
    std::iter_swap(a, b);
}

// Minimal wrapper for iterator to enable ADL
template <typename T>
struct Iterator_Wrapper
{
    using iterator = typename std::vector<T>::iterator;
    using iterator_category = typename std::iterator_traits<iterator>::iterator_category;
    using value_type = typename std::iterator_traits<iterator>::value_type;
    using difference_type = typename std::iterator_traits<iterator>::difference_type;
    using pointer = typename std::iterator_traits<iterator>::pointer;
    using reference = typename std::iterator_traits<iterator>::reference;

    iterator iter;
    iterator base_iter; // Track base for position calculation

    Iterator_Wrapper() = default;
    Iterator_Wrapper(iterator it, iterator base = iterator()) : iter(it), base_iter(base) { }

    reference operator*() const { return *iter; }
    pointer operator->() const { return iter.operator->(); }

    Iterator_Wrapper &operator++()
    {
        ++iter;
        return *this;
    }
    Iterator_Wrapper operator++(int)
    {
        auto tmp = *this;
        ++iter;
        return tmp;
    }
    Iterator_Wrapper &operator--()
    {
        --iter;
        return *this;
    }
    Iterator_Wrapper operator--(int)
    {
        auto tmp = *this;
        --iter;
        return tmp;
    }

    Iterator_Wrapper &operator+=(difference_type n)
    {
        iter += n;
        return *this;
    }
    Iterator_Wrapper &operator-=(difference_type n)
    {
        iter -= n;
        return *this;
    }

    Iterator_Wrapper operator+(difference_type n) const
    {
        return Iterator_Wrapper(iter + n, base_iter);
    }
    Iterator_Wrapper operator-(difference_type n) const
    {
        return Iterator_Wrapper(iter - n, base_iter);
    }

    difference_type operator-(const Iterator_Wrapper &other) const { return iter - other.iter; }

    reference operator[](difference_type n) const { return iter[n]; }

    bool operator==(const Iterator_Wrapper &other) const { return iter == other.iter; }
    bool operator!=(const Iterator_Wrapper &other) const { return iter != other.iter; }
    bool operator<(const Iterator_Wrapper &other) const { return iter < other.iter; }
    bool operator<=(const Iterator_Wrapper &other) const { return iter <= other.iter; }
    bool operator>(const Iterator_Wrapper &other) const { return iter > other.iter; }
    bool operator>=(const Iterator_Wrapper &other) const { return iter >= other.iter; }

    iterator base() const { return iter; }
};

template <typename T>
Iterator_Wrapper<T> operator+(typename Iterator_Wrapper<T>::difference_type n,
                              const Iterator_Wrapper<T> &it)
{
    return it + n;
}

// ADL-enabled iter_swap for our wrapper type
template <typename T>
void iter_swap(Iterator_Wrapper<T> a, Iterator_Wrapper<T> b)
{
    if (g_swap_log) {
        int pos_a = static_cast<int>(a.iter - a.base_iter);
        int pos_b = static_cast<int>(b.iter - b.base_iter);
        g_swap_log->push_back({ pos_a, pos_b });
    }
    std::iter_swap(a.iter, b.iter);
}

} // namespace adl_test

TEST_F(Minmax_Heap_Test, ADL_Iter_Swap_Make_Heap)
{
    using namespace adl_test;

    std::vector<int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    std::vector<std::pair<int, int>> swap_log;

    // Set up ADL tracking
    g_swap_log = &swap_log;

    Iterator_Wrapper<int> first(v.begin(), v.begin());
    Iterator_Wrapper<int> last(v.end(), v.begin());

    test_make(first, last);

    // Verify that swaps were logged (make_heap should perform some swaps)
    EXPECT_GT(swap_log.size(), 0u) << "ADL iter_swap should have been called";

    // Clean up
    g_swap_log = nullptr;
}

TEST_F(Minmax_Heap_Test, ADL_Iter_Swap)
{
    using namespace adl_test;

    // Prepare initial state
    std::vector<int> v = { 1, 5, 3 };
    std::vector<std::pair<int, int>> swap_log;
    Iterator_Wrapper<int> first;
    Iterator_Wrapper<int> last;

    // Push element
    swap_log.clear();
    g_swap_log = &swap_log;
    v.push_back(-1);
    first = Iterator_Wrapper<int>(v.begin(), v.begin());
    last = Iterator_Wrapper<int>(v.end(), v.begin());
    test_push(first, last - 1, -1);

    EXPECT_GT(swap_log.size(), 0u) << "ADL iter_swap should have been called during push";

    // Pop min element
    swap_log.clear();
    g_swap_log = &swap_log;
    first = Iterator_Wrapper<int>(v.begin(), v.begin());
    last = Iterator_Wrapper<int>(v.end(), v.begin());
    test_pop_min(first, last);
    v.pop_back();

    EXPECT_GT(swap_log.size(), 0u) << "ADL iter_swap should have been called during pop-min";

    // Pop max element
    swap_log.clear();
    g_swap_log = &swap_log;
    first = Iterator_Wrapper<int>(v.begin(), v.begin());
    last = Iterator_Wrapper<int>(v.end(), v.begin());
    test_pop_max(first, last);
    v.pop_back();

    EXPECT_GT(swap_log.size(), 0u) << "ADL iter_swap should have been called during pop-max";

    // Clean up
    g_swap_log = nullptr;
}

// Test ADL iter_swap with multiple operations
TEST_F(Minmax_Heap_Test, ADL_Iter_Swap_Multiple_Operations)
{
    using namespace adl_test;

    std::vector<int> v = { 5 };
    std::vector<std::pair<int, int>> total_swap_log;

    g_swap_log = &total_swap_log;
    Iterator_Wrapper<int> first(v.begin(), v.begin());
    Iterator_Wrapper<int> last(v.end(), v.begin());
    make_minmax_heap(first, last);

    // Push several elements
    std::vector<int> to_insert = { 3, 7, 1, 9, 2, 8 };
    for (int val : to_insert) {
        v.push_back(val);
        Iterator_Wrapper<int> push_first(v.begin(), v.begin());
        Iterator_Wrapper<int> push_last(v.end(), v.begin());
        push_minmax_heap(push_first, push_last);
    }

    // Verify final heap
    EXPECT_TRUE(is_minmax_heap(v.begin(), v.end()));

    // Verify comprehensive tracking
    EXPECT_GT(total_swap_log.size(), 0u)
            << "Multiple operations should generate swap tracking data";

    g_swap_log = nullptr;
}

// ============================================================================
// ADL Tests with Raw Vector Iterators (Custom Type)
// These tests demonstrate that ADL DOES work with std::vector<T>::iterator
// when T is a user-defined type in a user namespace. ADL will look in the
// namespace of T, so our custom iter_swap is found.
// ============================================================================

TEST_F(Minmax_Heap_Test, ADL_Raw_Iterator_Custom_Type_Make_Heap)
{
    using namespace adl_test;

    std::vector<Tracked_Int> v = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
    std::vector<std::pair<int, int>> swap_log;

    g_swap_log = &swap_log;
    g_tracked_int_base_iter = v.begin();

    test_make(v.begin(), v.end());
    EXPECT_GT(swap_log.size(), 0u) << "Custom iter_swap should be called via ADL";

    g_swap_log = nullptr;
}

TEST_F(Minmax_Heap_Test, ADL_Raw_Iterator_Custom_Type_Detailed_Tracking)
{
    using namespace adl_test;

    std::vector<Tracked_Int> v = { 3, 1, 2 };
    std::vector<std::pair<int, int>> swap_log;

    g_swap_log = &swap_log;
    g_tracked_int_base_iter = v.begin();

    test_make(v.begin(), v.end());

    expect_valid_property(v.begin(), v.end());
    EXPECT_GT(swap_log.size(), 0u);

    // Verify all swap indices are valid
    for (const auto &swap_pair : swap_log) {
        EXPECT_GE(swap_pair.first, 0);
        EXPECT_LT(swap_pair.first, static_cast<int>(v.size()));
        EXPECT_GE(swap_pair.second, 0);
        EXPECT_LT(swap_pair.second, static_cast<int>(v.size()));
    }

    g_swap_log = nullptr;
}

} // namespace qpmu::testing
