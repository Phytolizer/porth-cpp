#pragma once

#if __cplusplus > 201703L
#include <algorithm>
#include <ranges>

namespace my_ranges {
template <std::ranges::input_range Range, std::weakly_incrementable Out>
requires std::indirectly_copyable<std::ranges::iterator_t<Range>, Out>
constexpr std::ranges::copy_result<std::ranges::borrowed_iterator_t<Range>, Out> copy(Range&& range, Out out) {
    return std::ranges::copy(std::move(range), out);
}
} // namespace my_ranges
#else
#include <range/v3/algorithm/copy.hpp>

namespace my_ranges {
/// \overload
template <typename Rng, typename O>
/// \pre
requires ranges::input_range<Rng> && ranges::weakly_incrementable<O> &&
    ranges::indirectly_copyable<ranges::iterator_t<Rng>, O>
constexpr ranges::copy_result<ranges::borrowed_iterator_t<Rng>, O> copy(Rng&& rng, O out) {
    return ranges::copy(rng, out);
}
} // namespace my_ranges
#endif
