#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <array>
#include <functional>

namespace util
{

// Evenly spaced values including both endpoints.
// n == 1 returns {start}.
template <typename T>
std::vector<T> linspace(T start, T stop, std::size_t n) {
    std::vector<T> out;
    if (n == 0) return out;
    out.reserve(n);
    if (n == 1) {
        out.push_back(start);
        return out;
    }
    // For floating-point T, do arithmetic in double for stability
    if constexpr (std::is_floating_point_v<T>) {
        double ds = static_cast<double>(start);
        double de = static_cast<double>(stop);
        double step = (de - ds) / static_cast<double>(n - 1);
        for (std::size_t i = 0; i < n; ++i) {
            out.push_back(static_cast<T>(ds + step * static_cast<double>(i)));
        }
    } else {
        // Integral type (e.g., uint32_t): we round to nearest uint at each step.
        long double ds = static_cast<long double>(start);
        long double de = static_cast<long double>(stop);
        long double step = (de - ds) / static_cast<long double>(n - 1);
        for (std::size_t i = 0; i < n; ++i) {
            long double v = ds + step * static_cast<long double>(i);
            auto rounded = static_cast<long long>(std::llround(v));
            if constexpr (std::is_unsigned_v<T>) {
                out.push_back(static_cast<T>(rounded < 0 ? 0 : static_cast<unsigned long long>(rounded)));
            } else {
                out.push_back(static_cast<T>(rounded));
            }
        }
        // Optional: de-duplicate if rounding created repeats (e.g., small ranges)
        out.erase(std::unique(out.begin(), out.end()), out.end());
    }
    return out;
}

// Generic Cartesian product odometer over N dimensions.
// Takes a vector of "dimensions" where each dimension is the size of that axis.
// Calls 'fn' with a vector of indices (one per dimension) for each combination.
template <typename Fn>
void cartesian_product(const std::vector<std::size_t>& dims, Fn fn) {
    if (dims.empty()) {
        fn(std::vector<std::size_t>{});
        return;
    }
    std::vector<std::size_t> idx(dims.size(), 0);
    while (true) {
        fn(idx); // Visit current combination
        // Increment odometer (rightmost index fastest)
        std::size_t k = dims.size();
        while (k > 0) {
            --k;
            if (++idx[k] < dims[k]) break;
            idx[k] = 0;
        }
        if (k == 0 && idx[0] == 0) break; // wrapped around: done
    }
}

template <typename T> inline T CalcDiff(T n, const T &start, const T &stop)
{
    if (start == stop)
    {
        return 0;
    }

    return (stop - start) / n;
}
}