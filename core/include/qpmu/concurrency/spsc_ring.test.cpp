#include <gtest/gtest.h>

#include "spsc_ring.hpp"

#include <atomic>
#include <thread>
#include <vector>

namespace qpmu::testing {

class SPSC_Ring_Test : public ::testing::Test
{
};

TEST_F(SPSC_Ring_Test, Basic_Push_Pop)
{
    SPSC_Ring<int, 4> ring;
    int out = -1;

    EXPECT_TRUE(ring.try_push(42));
    EXPECT_TRUE(ring.try_pop(out));
    EXPECT_EQ(out, 42);
}

TEST_F(SPSC_Ring_Test, Empty_Pop_Returns_False)
{
    SPSC_Ring<int, 4> ring;
    int out = -1;

    EXPECT_FALSE(ring.try_pop(out));
    EXPECT_EQ(out, -1);
}

TEST_F(SPSC_Ring_Test, Full_Ring_Push_Returns_False)
{
    SPSC_Ring<int, 4> ring;

    EXPECT_TRUE(ring.try_push(1));
    EXPECT_TRUE(ring.try_push(2));
    EXPECT_TRUE(ring.try_push(3));
    EXPECT_FALSE(ring.try_push(4));
}

TEST_F(SPSC_Ring_Test, FIFO_Order)
{
    SPSC_Ring<int, 8> ring;

    for (int i = 0; i < 7; ++i)
        ASSERT_TRUE(ring.try_push(i));

    for (int i = 0; i < 7; ++i) {
        int out = -1;
        ASSERT_TRUE(ring.try_pop(out));
        EXPECT_EQ(out, i);
    }
}

TEST_F(SPSC_Ring_Test, Size_Approx)
{
    SPSC_Ring<int, 8> ring;
    EXPECT_EQ(ring.size_approx(), 0u);

    for (int i = 0; i < 5; ++i)
        ring.try_push(i);
    EXPECT_EQ(ring.size_approx(), 5u);

    int out;
    ring.try_pop(out);
    ring.try_pop(out);
    EXPECT_EQ(ring.size_approx(), 3u);

    for (int i = 0; i < 3; ++i)
        ring.try_pop(out);
    EXPECT_EQ(ring.size_approx(), 0u);
}

TEST_F(SPSC_Ring_Test, Wrap_Around)
{
    SPSC_Ring<int, 4> ring;

    for (int i = 0; i < 3; ++i)
        ASSERT_TRUE(ring.try_push(i));
    int out;
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(ring.try_pop(out));
        EXPECT_EQ(out, i);
    }

    for (int i = 10; i < 13; ++i)
        ASSERT_TRUE(ring.try_push(i));
    for (int i = 10; i < 13; ++i) {
        ASSERT_TRUE(ring.try_pop(out));
        EXPECT_EQ(out, i);
    }
}

TEST_F(SPSC_Ring_Test, Multiple_Wrap_Arounds)
{
    SPSC_Ring<int, 4> ring;
    int out;

    for (int round = 0; round < 20; ++round) {
        int base = round * 3;
        for (int i = 0; i < 3; ++i)
            ASSERT_TRUE(ring.try_push(base + i)) << "round=" << round << " i=" << i;
        for (int i = 0; i < 3; ++i) {
            ASSERT_TRUE(ring.try_pop(out)) << "round=" << round << " i=" << i;
            EXPECT_EQ(out, base + i);
        }
        EXPECT_EQ(ring.size_approx(), 0u);
    }
}

TEST_F(SPSC_Ring_Test, Multithreaded_Producer_Consumer)
{
    constexpr int N = 100'000;
    SPSC_Ring<int, 1024> ring;
    std::vector<int> received;
    received.reserve(N);

    std::atomic<bool> done{ false };

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            while (!ring.try_push(i))
                std::this_thread::yield();
        }
        done.store(true, std::memory_order_release);
    });

    std::thread consumer([&] {
        int val;
        while (true) {
            if (ring.try_pop(val)) {
                received.push_back(val);
            } else if (done.load(std::memory_order_acquire)) {
                while (ring.try_pop(val))
                    received.push_back(val);
                break;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(static_cast<int>(received.size()), N);
    for (int i = 0; i < N; ++i)
        EXPECT_EQ(received[i], i) << "mismatch at index " << i;
}

} // namespace qpmu::testing
