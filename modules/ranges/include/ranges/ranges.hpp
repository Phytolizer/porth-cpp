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

} // namespace my_ranges
#endif
