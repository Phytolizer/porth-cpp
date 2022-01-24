#pragma once

#if __cplusplus > 201703L && __has_include(<ranges>)
#include <algorithm>
#include <ranges>

namespace my_ranges = std::ranges;
#else
#include <range/v3/algorithm/copy.hpp>

namespace my_ranges = ranges;
#endif
