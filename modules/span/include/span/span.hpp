#pragma once

#include <cstddef>
#if __has_include(<span>)
#include <span>

namespace span {
template <typename T, std::size_t Sz = std::dynamic_extent> using Span = std::span<T, Sz>;
}
#else
#include <gsl/span>

namespace span {
template <typename T, std::size_t Sz = gsl::dynamic_extent> using Span = gsl::span<T, Sz>;
}
#endif
