#pragma once

#include "types.h"
#include "macros.h"

template<typename T> constexpr auto Mul32(T a, T b) requires(std::is_integral<T>::value) { 
    if constexpr(std::is_signed<T>::value) return int32(a) * int32(b);
    else return uint32(a) * uint32(b);
}

template<typename T> constexpr auto Mul64(T a, T b) requires(std::is_integral<T>::value && sizeof(T) < 4) {
    
    auto result = Mul32(a, b);

    if constexpr(std::is_signed<T>::value) return int64(result);
    else return uint64(result);
}

DEFINE_FUNC(__muldi3,
    constexpr USED int64 Mul64(int64 a, int64 b) 
) {

    // TODO: Optimize this 
    uint16 a0 = a>>48;
    uint16 a1 = a>>32;
    uint16 a2 = a>>16;
    uint16 a3 = a;

    uint16 b0 = b>>48;
    uint16 b1 = b>>32;
    uint16 b2 = b>>16;
    uint16 b3 = b;

    // Note: from the expansion of (a0<<48):(a1<<32):(a2<<16):(a3<<0) * (a0<<48):(a1<<32):(a2<<16):(a3<<0)
    //       we ignore the 2^96, 2^80, and 2^64 terms because we are only calculating lower 64 bits of multiplication
    uint32 c0  = Mul32(a3,b3);
    uint64 c16 = uint64(Mul32(a3, b2)) + Mul32(b3, a2);
    uint64 c32 = uint64(Mul32(a1, b3)) + Mul32(b1, a3) + Mul32(a2, b2);
    uint64 c48 = uint64(Mul32(a0, b3)) + Mul32(b0, a3) + Mul32(a1, b2) + Mul32(b1, a2); 

    return (c48<<48) + (c32<<32) + (c16<<16) + c0;
}

//// 32-bit routines
// PROVIDE ( __divsi3 = 0x4000dc88 );     __divsi3 (int32 a, int32 b) return a/b
// PROVIDE ( __udivsi3 = 0x4000e21c );    __udivsi3 (uint32 a, uint32 b) return a/b
// PROVIDE ( __umodsi3 = 0x4000e268 );    __umodsi3 (uint32 a, uint32 b) return a%b
// PROVIDE ( __fixdfsi = 0x4000ccb8 );    __fixdfsi (double a) return int32(a)
// PROVIDE ( __fixunsdfsi = 0x4000cd00 ); __fixunsdfsi (double a) return uint32(a) 
// PROVIDE ( __fixunssfsi = 0x4000c4c4 ); __fixunssfsi (float a) return uint32(a)

//// 64-bit routines
// PROVIDE ( __muldi3 = 0x40000650 );    __muldi3 (int64 a, int64 b) return a*b
// PROVIDE ( __divdi3 = 0x4000ce60 );    __divdi3 (int64 a, int64 b) return a/b
// PROVIDE ( __udivdi3 = 0x4000d310 );   __udivdi3 (uint64 a, uint64 b) return a/b
// PROVIDE ( __umoddi3 = 0x4000d770 );   __umoddi3 (uint64 a, uint64 b) return a%b
// PROVIDE ( __umulsidi3 = 0x4000dcf0 ); __umulsidi3 (uint32 a, uint32 b) return uint64(a) * uint64(b)

//// float routines
// PROVIDE ( __addsf3 = 0x4000c180 );       __addsf3 (float a, float b) return a+b
// PROVIDE ( __subsf3 = 0x4000c268 );       __subsf3 (float a, float b) return a-b
// PROVIDE ( __mulsf3 = 0x4000c3dc );       __mulsf3 (float a, float b) return a*b
// PROVIDE ( __floatsisf = 0x4000e2ac );    __floatsisf (int32 i) return float(i)
// PROVIDE ( __truncdfsf2 = 0x4000cd5c );   __truncdfsf2 (double a) return float(a)
// PROVIDE ( __floatunsisf = 0x4000e2a4 );  __floatunsisf (uint32 i) return float(i)

//// double routines
// PROVIDE ( __adddf3 = 0x4000c538 );       __adddf3 (double a, double b) return a+b
// PROVIDE ( __subdf3 = 0x4000c688 );       __subdf3 (double a, double b) return a-b
// PROVIDE ( __muldf3 = 0x4000c8f0 );       __muldf3 (double a, double b) return a*b
// PROVIDE ( __divdf3 = 0x4000cb94 );       __divdf3 (double a, double b) return a/b
// PROVIDE ( __floatsidf = 0x4000e2f0 );    __floatsidf (int32 i) return double(i)
// PROVIDE ( __floatunsidf = 0x4000e2e8 );  __floatunsidf (uint32 i) return double(i)
// PROVIDE ( __extendsfdf2 = 0x4000cdfc );  __extendsfdf2 (float a) return double(a)
