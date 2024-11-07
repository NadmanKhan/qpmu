#ifndef QPMU_PHASOR_ESTIMATOR_SZCC_H
#define QPMU_PHASOR_ESTIMATOR_SZCC_H

#include "qpmu/defs.h"

#include "atcoder/segtree.hpp"

#include <cmath>
#include <complex>
#include <vector>

namespace qpmu {

namespace detail {
template <class Integral>
constexpr Integral select(bool cond, const Integral &a, const Integral &b)
{
    return (cond * a) + (!cond * b);
}

template <class Integral>
constexpr int sgn(const Integral &x)
{
    return int(x > 0) - int(x < 0);
}

template <class SampleType, class TimeType>
constexpr TimeType zeroCrossingTime(TimeType t0, SampleType x0, TimeType t1, SampleType x1)
{
    return t0 + (0 - x0) * (t1 - t0) / (x1 - x0);
}

template <class T>
constexpr T fmax(T x, T y)
{
    return std::max(x, y);
}

template <class T>
constexpr T fmin(T x, T y)
{
    return std::min(x, y);
}
} // namespace detail

template <class T>
using RangeMinSegTree = atcoder::segtree<T, detail::fmin<T>, std::numeric_limits<T>::max>;
template <class T>
using RangeMaxSegTree = atcoder::segtree<T, detail::fmax<T>, std::numeric_limits<T>::min>;

template <class SampleTypeIntegral>
class SlidingZeroCrossingCounter
{
public:
    using SampleType = SampleTypeIntegral;
    using TimeType = uint64_t;

    SlidingZeroCrossingCounter() = delete;
    SlidingZeroCrossingCounter(const SlidingZeroCrossingCounter &) = default;
    SlidingZeroCrossingCounter(SlidingZeroCrossingCounter &&) = default;
    SlidingZeroCrossingCounter &operator=(const SlidingZeroCrossingCounter &) = default;
    SlidingZeroCrossingCounter &operator=(SlidingZeroCrossingCounter &&) = default;

    SlidingZeroCrossingCounter(size_t n, uint64_t interval)
        : m_ts(n, 0),
          m_xs(n, 0),
          m_xRangeMinTree(n),
          m_xRangeMaxTree(n),
          m_zcCounts(n, 0),
          m_zcLastndex(n, 0),
          m_interval(interval)
    {
    }

    void addSample(TimeType t, SampleType x)
    {
        auto tailCurr = m_tail;
        auto tailPrev = prevPos(m_tail);
        auto tailNext = nextPos(m_tail);
        m_tail = tailNext;

        /// Update the time and sample at the end pointer.
        set(tailCurr, t, x);
        m_zcCounts[tailCurr] = m_zcCounts[tailPrev];
        m_zcLastndex[tailCurr] = m_zcLastndex[tailPrev];

        /// Use two pointers to keep track of the start and end of the window such that ---
        /// 1. the time difference between the start and end samples is less than or equal to the
        ///    interval, and
        /// 2. a zero-crossing has occurred at the start sample.
        /// Move the start pointer to the right until conditions are satisfied.
        while ((m_ts[m_tail] - m_ts[m_head] > TimeType(m_interval))
               || (m_zcCounts[m_head] == m_zcCounts[prevPos(m_head)])) {
            if (nextPos(m_head) == m_tail) {
                break;
            }
            m_head = nextPos(m_head);
        }

        /// Update the zero-crossing count.
        /// If the sign of the current sample is different from the previous sample, then a
        /// zero-crossing has occurred.
        auto [wmin, wmax] = rangeMinMax();
        int64_t wmid = (wmin + wmax) / 2;
        auto t0 = m_ts[tailPrev];
        auto x0 = m_xs[tailPrev];
        auto t1 = m_ts[m_tail];
        auto x1 = m_xs[m_tail];
        if (detail::sgn(int64_t(x0) - wmid) != detail::sgn(int64_t(x1) - wmid)) {
            /// A zero-crossing has occurred, so update the zero-crossing count,
            /// and compute the zero-crossing time insert another sample at that time.
            m_zcCounts[m_tail] = m_zcCounts[tailPrev] + 1;
            auto tzc = detail::zeroCrossingTime(t0, x0, t1, x1);
            set(tzc, 0);
            /// Move the end pointer to the right and update the time and sample at the new end
            /// pointer.
            m_zcLastndex[tailNext] = m_tail;
            m_tail = tailNext;
            set(t, x);
        }

        /// Move the end pointer to the right.
        m_tail = nextPos(m_tail);
    }

    constexpr size_t size() const { return m_ts.size(); }

    constexpr uint64_t interval() const { return m_interval; }

    constexpr std::pair<size_t, size_t> currentRange() const { return std::pair(m_head, m_tail); }

    constexpr size_t rangeSize(size_t l, size_t r) const
    {
        return (r + detail::select(l <= r, size_t(0), m_ts.size())) - l;
    }

    constexpr std::pair<SampleType, SampleType> rangeMinMax(size_t l, size_t r) const
    {
        if (l <= r) {
            return std::pair(m_xRangeMinTree.prod(l, r), m_xRangeMaxTree.prod(l, r));
        } else {
            auto min1 = m_xRangeMinTree.prod(l, m_ts.size());
            auto min2 = m_xRangeMinTree.prod(0, r);
            auto max1 = m_xRangeMaxTree.prod(l, m_ts.size());
            auto max2 = m_xRangeMaxTree.prod(0, r);
            return std::pair(std::min(min1, min2), std::max(max1, max2));
        }
    }

    constexpr TimeType rangeTime(size_t l, size_t r) const
    {
        r = m_zcLastndex[r];
        if (m_ts[r] < m_ts[l]) {
            r = l;
        }
        return m_ts[r] - m_ts[l];
    }

    constexpr uint64_t windowZC() const
    {
        return m_zcCounts[prevPos(m_tail)] - m_zcCounts[prevPos(m_head)];
    }

    constexpr const TimeType &time(size_t i) const { return m_ts[(m_head + i) % m_ts.size()]; }

    constexpr const SampleType &sample(size_t i) const { return m_xs[(m_head + i) % m_ts.size()]; }

    constexpr const TimeType &lastTime() const { return m_ts[prevPos(m_tail)]; }

    constexpr const SampleType &lastSample() const { return m_xs[prevPos(m_tail)]; }

    constexpr const RangeMinSegTree<SampleType> &xRangeMinTree() const { return m_xRangeMinTree; }

    constexpr const RangeMaxSegTree<SampleType> &xRangeMaxTree() const { return m_xRangeMaxTree; }

    constexpr const uint64_t &zcCounts(size_t i) const
    {
        return m_zcCounts[(m_head + i) % m_ts.size()];
    }

    constexpr const size_t &zcLastndex(size_t i) const
    {
        return m_zcLastndex[(m_head + i) % m_ts.size()];
    }

private:
    constexpr size_t prevPos(size_t pos) const
    {
        return detail::select(pos == 0, m_ts.size() - 1, pos - 1);
    }

    constexpr size_t nextPos(size_t pos) const
    {
        return detail::select(pos + 1 == m_ts.size(), size_t(0), pos + 1);
    }

    void set(size_t pos, TimeType t, SampleType x)
    {
        m_ts[pos] = t;
        m_xs[pos] = x;
        m_xRangeMinTree.set(pos, x);
        m_xRangeMaxTree.set(pos, x);
    }

private:
    std::vector<TimeType> m_ts;
    std::vector<SampleType> m_xs;
    RangeMinSegTree<SampleType> m_xRangeMinTree;
    RangeMaxSegTree<SampleType> m_xRangeMaxTree;
    std::vector<uint64_t> m_zcCounts;
    std::vector<size_t> m_zcLastndex;

    /// Time interval for counting zero-crossings
    uint64_t m_interval = 0;

    /// Start index of the current window (inclusive)
    size_t m_head = 0;

    /// End index of the current window (exclusive)
    size_t m_tail = 0;
};

} // namespace qpmu

#endif // QPMU_PHASOR_ESTIMATOR_SZCC_H