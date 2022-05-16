#pragma once

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

template<typename T1, typename T2>
constexpr auto Lerp(float t, T1 v1, T2 v2) {
    return t*(v2 - v1) + v1;
}