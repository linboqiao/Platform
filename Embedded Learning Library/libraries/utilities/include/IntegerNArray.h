////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     IntegerNArray.h (utilities)
//  Authors:  Kern Handa
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

// stl
#include <array>
#include <utility>

namespace ell
{
namespace utilities
{
    namespace detail
    {
        template <size_t N, size_t... I>
        constexpr auto MakeNArrayImpl(std::index_sequence<I...>)
        {
            return std::array<int, N>{ I... };
        }
    }

    /// <summary> Returns a compile-time array of N integers, filled with values 0 .. N-1 </summary>
    ///
    /// <typeparam name="N"> The size of the array to generate. </typeparam>
    template <size_t N>
    constexpr auto MakeNArray()
    {
        return detail::MakeNArrayImpl<N>(std::make_index_sequence<N>{});
    }
}
}
