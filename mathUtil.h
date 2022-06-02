#pragma once

#include "types.h"
#include <cmath>

template<typename T1, typename T2>
constexpr auto Max(T1 a, T2 b) {
    return a >= b ? a : b;
}

template<typename T1, typename T2>
constexpr auto Min(T1 a, T2 b) {
    return a <= b ? a : b;
}

template<typename T1, typename T2, typename T3>
constexpr auto Clamp(T1 value, T2 min, T3 max) {
    return (value <= min) ? min : (value >= max) ? max : value;
}

template<typename T1, typename T2, typename T3>
constexpr auto Lerp(T1 t, T2 v1, T3 v2) {
    return t*(v2 - v1) + v1;
}

template<typename T>
constexpr bool SignBit(T n) { return signbit(n); }

template<typename T1, typename T2>
constexpr bool SameSignBit(T1 a, T2 b) {    
    
    if constexpr(std::is_unsigned<T1>::value && std::is_unsigned<T2>::value) {

        return true;

    } else if constexpr(std::is_integral<T1>::value && std::is_integral<T2>::value) {

        auto aXorB = a ^ b;
        using UnsignedT = std::make_unsigned<decltype(aXorB)>::type;

        return ~UnsignedT(aXorB) >> (8*sizeof(UnsignedT) - 1);
    
    } else {

        return SignBit(a) == SignBit(b);

    }
}