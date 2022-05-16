#pragma once

#include "types.h"

template<bool kSigned, uint8 kBits> struct SpanningInteger {

    static inline constexpr auto GetType() {
        
             if constexpr(kBits <=  8) return typename SpanningInteger<kSigned,  8>::type{};
        else if constexpr(kBits <= 16) return typename SpanningInteger<kSigned, 16>::type{};
        else if constexpr(kBits <= 32) return typename SpanningInteger<kSigned, 32>::type{};
        else if constexpr(kBits <= 64) return typename SpanningInteger<kSigned, 64>::type{};
        else static_assert(kBits != kBits, "Unsupported number of bits");
    }

    using type = decltype(GetType());
};

template<> struct SpanningInteger<true,  8> { using type = int8; };
template<> struct SpanningInteger<true, 16> { using type = int16; };
template<> struct SpanningInteger<true, 32> { using type = int32; };
template<> struct SpanningInteger<true, 64> { using type = int64; };

template<> struct SpanningInteger<false,  8> { using type = uint8; };
template<> struct SpanningInteger<false, 16> { using type = uint16; };
template<> struct SpanningInteger<false, 32> { using type = uint32; };
template<> struct SpanningInteger<false, 64> { using type = uint64; };