#pragma once

#include "types.h"
#include "macros.h"

#include "mathUtil.h"
#include "softMath.h"
#include "FixedPoint.h"

// TODO: Move this into more extensive logging header
#include <SoftwareSerial.h>
#define Log(msg, fmt...) Serial.printf("MSG - " msg "\n", fmt)
#define Warn(msg, fmt...) Serial.printf("WARN - " msg "\n", fmt)

template<typename T, size_t kN>
inline constexpr size_t ArrayCount(const T (&)[kN]) { return kN; }

template<typename T, typename TBytes>
inline constexpr const void* ByteOffset(const T* ptr, TBytes bytes) {
    return reinterpret_cast<const char*>(ptr) + bytes;
}

template<typename T, typename TBytes>
inline constexpr void* ByteOffset(T* ptr, TBytes bytes) {
    return reinterpret_cast<char*>(ptr) + bytes;
}

template<typename T>
constexpr T BigEndian(T val) {

    constexpr size_t kSize = sizeof(T);

         if constexpr(kSize == 1)  return val;
    else if constexpr(kSize == 2)  return __builtin_bswap16(val);
    else if constexpr(kSize == 4)  return __builtin_bswap32(val);
    else if constexpr(kSize == 8)  return __builtin_bswap64(val);
    else if constexpr(kSize == 16) return __builtin_bswap128(val);   
    else static_assert(kSize != kSize, "Unsupported Type");
}

template<typename T>
constexpr T LittleEndian(T val) { return val; }

// TODO: Move this out to stringUtil.h
#include <WString.h>

// Hack to prevent intelisense failing to infer templates in WString.h
#ifdef __INTELLISENSE__
    template<typename T>
    inline String operator+(const String& str, T arg) {
        String result = String(str).concat(arg);
        return result;
    }
#endif

template<typename LambdaT>
auto CriticalSection(LambdaT lambda) {
    noInterrupts();

    using ReturnT = decltype(lambda());
    if constexpr(std::is_same<ReturnT, void>::value) {

        lambda();    
        interrupts();
        return;
    
    } else {
        
        auto result = lambda();
        interrupts();
        return result;
    }
} 

template<typename DeferT, typename ValT>
constexpr ValT Defer(ValT val) { return val; }
