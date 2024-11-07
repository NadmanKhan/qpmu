#include "estimator/szcc.hpp"

#include <gtest/gtest.h>

TEST(SZCC, ConstructorsAndAssignment)
{
    qpmu::SlidingZeroCrossingCounter<int> a(44100, 1000);
    qpmu::SlidingZeroCrossingCounter<int> b(a);
    qpmu::SlidingZeroCrossingCounter<int> c(std::move(a));
    a = b;
    a = std::move(c);

    ASSERT_EQ(a.size(), b.size());
}
