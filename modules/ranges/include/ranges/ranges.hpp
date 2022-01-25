#pragma once

#if __cplusplus > 201703L && __has_include(<ranges>)
#include <algorithm>
#include <ranges>

namespace my_ranges = std::ranges;
#else
// DIY

#include <algorithm>

namespace my_ranges {

template <typename Range, typename OutputIterator> constexpr OutputIterator copy(Range&& range, OutputIterator output) {
    return std::copy(range.begin(), range.end(), output);
}

template <typename Range, typename OutputIterator, typename Projection> constexpr OutputIterator transform(Range&& range, OutputIterator output, Projection projection) {
    return std::transform(range.begin(), range.end(), output, projection);
}

} // namespace my_ranges
#endif
