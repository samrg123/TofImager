#pragma once

#include "types.h"
#include "macros.h"

#include "metaProgramming.h"

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
constexpr auto ByteDistance(const T* ptr1, const T* ptr2) {
    return sizeof(T)*(ptr1 - ptr2);
}

inline auto ByteDistance(const void* ptr1, const void* ptr2) {
    return reinterpret_cast<const char*>(ptr1) - reinterpret_cast<const char*>(ptr2);
}

template<typename T>
constexpr void FastCopy(void* dst, const void* src, T bytes) { 
    
    uint8* dst8 = reinterpret_cast<uint8*>(dst);
    const uint8* src8 = reinterpret_cast<const uint8*>(src);

    // TODO: Optimize this
    // Note: It's worth having this loop here instead of explicity calling __builtin_memset.
    //       gcc will optimize this loop into __builin_memset via tree optimizations, but 
    //       providing source code also allows compiler to optimize based on context without 
    //       always inserting branch to memset.  

    for(T i = 0; i < bytes; ++i) {
        dst8[i] = src8[i];
    }
}

template<typename T>
constexpr bool IsPow2(T val) {
    return val && !(val & (val-1));
}

template<auto kN = 4, typename T>
constexpr T AlignLower(T ptr) {
    static_assert(IsPow2(kN), "kN must be a power of 2");
    return T(uintptr_t(ptr) & (~(kN-1)));
}

template<auto kN = 4, typename T>
constexpr bool IsAligned(T ptr) {
    return AlignLower<kN>(ptr) == ptr;
}

template<auto kN = 4, typename T>
constexpr T AlignUpper(T ptr) {
    return IsAligned<kN>(ptr) ? ptr : T(uintptr_t(AlignLower<kN>(ptr)) + kN);
}

template<typename T, typename ValueT>
constexpr void FastFill(T* start, T* end, const ValueT& value) {

    std::fill(start, end, value);

    // TODO: Get this to working - experiment with int32 packing to get
    //       this faster than std::fill {Check via godbolt}
    // Note: This may not be feasible until we get if conseval() in updated gcc

    // if constexpr(std::is_trivially_copyable<T>::value) {

    //     const T tValue = value;
    //     constexpr uint32 kTValueSize = sizeof(T);
    //     constexpr uint32 kTAlignment = alignof(T);

    //     uint32 bytes = ByteDistance(end, start);

    //     // Optimize small copies by packing value in 32 bit integer
    //     if constexpr(kTValueSize <= 4) {

    //         // align tValue to 32 bits;
    //         uint32 tVal32 = reinterpret_cast<const uint32&>(tValue);
    //         if constexpr(kTValueSize < 4) {

    //             if constexpr(kTValueSize == 1 || kTValueSize == 3) {
    //                 tVal32 = (tVal32 << 8) | tVal32;
    //             }

    //             if constexpr(kTValueSize != 3) {
    //                 tVal32|= tVal32<<16;
    //             }
    //         }

    //         // align start to 32 bits
    //         T* alignedStart;
    //         if constexpr(IsAligned<4>(kTAlignment)) {

    //             alignedStart = start;

    //         } else {

    //             alignedStart = AlignUpper<4>(start);

    //             uint8 unalignedBytes = ByteDistance(alignedStart, start);

    //             if(unalignedBytes && bytes >= unalignedBytes) {

    //                 FastCopy(start, &tVal32, unalignedBytes);
    //                 bytes-= unalignedBytes;

    //                 // offset tVal32
    //                 uint8 unalignedBits = 8*unalignedBytes;
    //                 tVal32 = (tVal32 << (32 - unalignedBits)) | (tVal32 >> unalignedBits);
    //             }
    //         }

    //         // copy raw bytes over
    //         uint32* start32 = reinterpret_cast<uint32*>(alignedStart);
    //         while(bytes >= 4) {
    //             *start32++ = tVal32;
    //             bytes-= 4;
    //         }

    //         // copy remaining bytes
    //         FastCopy(start32, &tVal32, bytes);

    //     } else {

    //         // TODO: Optimize this
    //         while(start < end) {
    //             FastCopy(start++, &tValue, kTValueSize);
    //         }
    //     }

    // } else {

    //     // Invoke assignment operator on each element
    //     while(start < end) {
    //         *start++ = value;
    //     }        
    // }
}

template<typename T>
OPTIMIZE("-fno-tree-loop-distribute-patterns")
inline constexpr void ByteMemset(T* dst, uint8 val, uint32 bytes) {
    
    uint8* dst8 = reinterpret_cast<uint8*>(dst);
    while(bytes) {
        *dst8++ = val; 
        --bytes;
    }
} 

template<typename T>
inline constexpr void FastMemset(T* dst, uint8 val, uint32 bytes) { 
    
    __builtin_memset(dst, val, bytes);

    // // TODO: Benchmark this against stdlib bootrom memset
    
    // // align val to 32 bits;
    // uint32 val32 = (uint32(val)<<8) | val;
    // val32|= val32<<16;

    // constexpr uint32 kTAlignment = alignof(T);
    // constexpr uint8  kTPossibleUnalignedBytes = kTAlignment%4;

    // // align dst to 32 bits
    // void* alignedDst;
    // if constexpr(!kTPossibleUnalignedBytes) { 
        
    //     alignedDst = dst;
    
    // } else {

    //     alignedDst = AlignUpper<4>(dst);
    //     uint8 bytesToAlign = ByteDistance(alignedDst, dst);

    //     if(bytesToAlign && bytesToAlign <= bytes) {

    //         // Copy over unaligned bytes
    //         ByteMemset(dst, val, bytesToAlign);
    //         bytes-= bytesToAlign;
    //     }
    // }
    
    // // 32 bit aligned memset
    // uint32* dst32 = reinterpret_cast<uint32*>(alignedDst);
    // while(bytes >= 4) {
    //     *dst32++ = val32;
    //     bytes-= 4;
    // }

    // // set remaining bytes
    // ByteMemset(dst32, val, bytes);
}

template<typename T>
constexpr T ByteSwap(T val) {

    constexpr size_t kSize = sizeof(T);

         if constexpr(kSize == 1)  return val;
    else if constexpr(kSize == 2)  return __builtin_bswap16(val);
    else if constexpr(kSize == 4)  return __builtin_bswap32(val);
    else if constexpr(kSize == 8)  return __builtin_bswap64(val);
    else if constexpr(kSize == 16) return __builtin_bswap128(val);   
    else static_assert(kSize != kSize, "Unsupported Type");
}

template<typename T>
constexpr T BigEndian(T val) { return ByteSwap(val); }

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

