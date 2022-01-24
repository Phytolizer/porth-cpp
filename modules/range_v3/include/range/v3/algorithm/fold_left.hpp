/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2021-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef RANGES_V3_ALGORITHM_FOLD_LEFT_HPP
#define RANGES_V3_ALGORITHM_FOLD_LEFT_HPP

#include <meta/meta.hpp>

#include <range/v3/functional/identity.hpp>
#include <range/v3/functional/invoke.hpp>
#include <range/v3/range/access.hpp>
#include <range/v3/range/concepts.hpp>
#include <range/v3/utility/optional.hpp>

#include <range/v3/detail/prologue.hpp>

namespace ranges
{
    // clang-format off
    template<class F, class T, class I, class U>
    CPP_concept indirectly_binary_left_foldable_impl =
        movable<T> &&
        movable<U> &&
        convertible_to<T, U> &&
        invocable<F&, U, iter_reference_t<I>> &&
        assignable_from<U&, invoke_result_t<F&, U, iter_reference_t<I>>>;

    template <class F, class T, class I>
    CPP_concept indirectly_binary_left_foldable =
        copy_constructible<F> &&
        indirectly_readable<I> &&
        invocable<F&, T, iter_reference_t<I>> &&
        convertible_to<invoke_result_t<F&, T, iter_reference_t<I>>,
          std::decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>> &&
        indirectly_binary_left_foldable_impl<F, T, I, std::decay_t<invoke_result_t<F&, T, iter_reference_t<I>>>>;
    // clang-format on

    /// \addtogroup group-algorithms
    /// @{
    struct fold_left_fn
    {
        template(typename I, typename S, typename T, typename Op)(
            /// \pre
            requires sentinel_for<S, I> AND input_iterator<I> AND
                indirectly_binary_left_foldable<Op, T, I>) //
            constexpr auto
            operator()(I first, S last, T init, Op op) const
        {
            using U = std::decay_t<invoke_result_t<Op &, T, iter_reference_t<I>>>;

            if(first == last)
            {
                return U(std::move(init));
            }

            U accum = invoke(op, std::move(init), *first);
            for(++first; first != last; ++first)
            {
                accum = invoke(op, std::move(accum), *first);
            }
            return accum;
        }

        template(typename Rng, typename T, typename Op)(
            /// \pre
            requires input_range<Rng> AND
                indirectly_binary_left_foldable<Op, T, iterator_t<Rng>>) //
            constexpr auto
            operator()(Rng && rng, T init, Op op) const
        {
            return (*this)(begin(rng), end(rng), std::move(init), std::move(op));
        }
    };

    struct fold_left_first_fn
    {
        template(typename I, typename S, typename Op)(
            /// \pre
            requires sentinel_for<S, I> AND input_iterator<I> AND
                indirectly_binary_left_foldable<Op, iter_value_t<I>, I>
                    AND constructible_from<iter_value_t<I>, iter_reference_t<I>>) //
            constexpr auto
            operator()(I first, S last, Op op) const
        {
            using U = invoke_result_t<fold_left_fn, I, S, iter_value_t<I>, Op>;
            if(first == last)
            {
                return optional<U>();
            }

            iter_value_t<I> init = *first;
            ++first;
            return optional<U>(
                in_place,
                fold_left_fn{}(std::move(first), std::move(last), std::move(init), op));
        }

        template(typename R, typename Op)(
            /// \pre
            requires input_range<R> AND
                indirectly_binary_left_foldable<Op, range_value_t<R>, iterator_t<R>>
                    AND constructible_from<range_value_t<R>, range_reference_t<R>>) //
            constexpr auto
            operator()(R && rng, Op op) const
        {
            return (*this)(begin(rng), end(rng), std::move(op));
        }
    };

    RANGES_INLINE_VARIABLE(fold_left_fn, fold_left)
    RANGES_INLINE_VARIABLE(fold_left_first_fn, fold_left_first)
    /// @}
} // namespace ranges

#include <range/v3/detail/epilogue.hpp>

#endif
