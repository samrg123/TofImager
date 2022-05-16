#pragma once

#include "types.h"
#include "SpanningInteger.h"
#include "mathUtil.h"

template<bool kSigned_, uint8 kIntegerBits_, uint8 kFractionBits_> 
struct FixedPoint {

    // TODO: add support for compile-time overflow detection

    static inline constexpr bool  kSigned       = kSigned_;
    static inline constexpr uint8 kIntegerBits  = kIntegerBits_;
    static inline constexpr uint8 kFractionBits = kFractionBits_;
    static inline constexpr uint8 kBits         = kSigned + kIntegerBits + kFractionBits;

    using BitsType     = SpanningInteger<false, kBits>::type;
    using IntegerType  = SpanningInteger<kSigned, kIntegerBits>::type;
    using FractionType = SpanningInteger<false, kFractionBits>::type;

    using FloatBitShiftT = SpanningInteger<false, kFractionBits+1>::type;
    static inline constexpr FloatBitShiftT kFloatBitShift = FloatBitShiftT(1) << kFractionBits;

    using MultiplicationT = SpanningInteger<false, 2*kBits>::type;

    BitsType bits : kBits;

    static inline constexpr FixedPoint FromBits(BitsType bits) {
        FixedPoint value;
        value.bits = bits;
        return value;
    }

    static inline constexpr BitsType MulBits(BitsType bits1, BitsType bits2) {
 
        // Note: (a*b)2^n = ((a2^b)(b2^n))/2^n
        if constexpr(std::is_same<MultiplicationT, uint64>::value) {
 
            // TODO: optimize this?
            // Explicitly call Mul64 so gcc can inline code
            return Mul64(bits1, bits2) >> kFractionBits; 
 
        } else {
            return (MultiplicationT(bits1) * MultiplicationT(bits2)) >> kFractionBits;
        }
    }

    static inline constexpr BitsType DivBits(BitsType bits1, BitsType bits2) {
        // Note: (a/b)2^n = ((a2^b)2^n)/(b2^n)
        return (MultiplicationT(bits1) << kFractionBits) / bits2;
    }    

    static inline constexpr auto kMinValue = FromBits(kSigned ? BitsType(1) << kFractionBits : 0);
    static inline constexpr auto kMaxValue = FromBits((~BitsType(0)) >> kSigned); 

    constexpr IntegerType  Integer()  const { return bits >> kFractionBits; }
    constexpr FractionType Fraction() const { return bits & ~(~BitsType(0) << kFractionBits); }

    constexpr FixedPoint() = default;

    template<bool kS, uint8 kI, uint8 kF>
    constexpr FixedPoint(FixedPoint<kS, kI, kF> v) {

        static_assert(kSigned >= kS,       "FixedPoint Construction Failed - constructed sign bits too small");
        static_assert(kIntegerBits >= kI,  "FixedPoint Construction Failed - constructed integer bits too small");
        static_assert(kFractionBits >= kF, "FixedPoint Construction Failed - constructed fraction bits too small");
    
        this->bits = BitsType(v.bits) << (kFractionBits - kF);
    }

    template<typename T> 
    constexpr FixedPoint(T n) {

        if constexpr(sizeof(T) > sizeof(BitsType)) {
            bits = BitsType(n << kFractionBits);
        } else {
            bits = BitsType(n) << kFractionBits;
        }
    }

    template<typename T1, typename T2> 
    constexpr FixedPoint(T1 numerator, T2 denominator) {
        using TmpT = SpanningInteger<std::is_signed<T1>::value, Min(64, 8*sizeof(T1) + kFractionBits) >::type;
        bits = BitsType( (TmpT(numerator)<<kFractionBits) / denominator );
    }


    // TODO: optimize with bitcast
    constexpr FixedPoint(double v) { bits = BitsType(v * kFloatBitShift); }
    constexpr FixedPoint(float  v) { bits = BitsType(v * kFloatBitShift); }

    template<typename T>
    constexpr explicit operator T() const { return Integer(); }

    // TODO: optimize with bitcast
    constexpr explicit operator float()  const { return bits / float(kFloatBitShift); }
    constexpr explicit operator double() const { return bits / double(kFloatBitShift); }
    
    constexpr bool operator<(FixedPoint v) const { return bits < v.bits; }
    constexpr bool operator>(FixedPoint v) const { return bits > v.bits; }

    constexpr bool operator<=(FixedPoint v) const { return bits <= v.bits; }
    constexpr bool operator>=(FixedPoint v) const { return bits >= v.bits; }    

    constexpr bool operator==(FixedPoint v) const { return bits == v.bits; }
    constexpr bool operator!=(FixedPoint v) const { return bits != v.bits; }

    // TODO: optimize integer multiply types? (No need to shift by power of 2) or does compiler already do that for us?

    constexpr FixedPoint operator+(FixedPoint n) const { return FromBits(bits + n.bits); }
    constexpr FixedPoint operator-(FixedPoint n) const { return FromBits(bits - n.bits); }
    constexpr FixedPoint operator*(FixedPoint n) const { return FromBits(MulBits(bits, n.bits)); }
    constexpr FixedPoint operator/(FixedPoint n) const { return FromBits(DivBits(bits, n.bits)); }

    constexpr FixedPoint& operator+=(FixedPoint n) { bits+= n.bits; return *this; }
    constexpr FixedPoint& operator-=(FixedPoint n) { bits-= n.bits; return *this; }
    constexpr FixedPoint& operator*=(FixedPoint n) { bits = MulBits(bits, n.bits); return *this; }
    constexpr FixedPoint& operator/=(FixedPoint n) { bits = DivBits(bits, n.bits); return *this; }

    template<typename T> friend constexpr FixedPoint operator+(T n, FixedPoint fixedPoint) { return fixedPoint + n; }
    template<typename T> friend constexpr FixedPoint operator-(T n, FixedPoint fixedPoint) { return FixedPoint(n) - fixedPoint; }
    template<typename T> friend constexpr FixedPoint operator*(T n, FixedPoint fixedPoint) { return fixedPoint * n; }
    template<typename T> friend constexpr FixedPoint operator/(T n, FixedPoint fixedPoint) { return FixedPoint(n) / fixedPoint; }
};

template<uint8 kIntegerBits_, uint8 kFractionBits_>
using SignedFixedPoint = FixedPoint<true, kIntegerBits_, kFractionBits_>;

template<uint8 kIntegerBits_, uint8 kFractionBits_>
using UnsignedFixedPoint = FixedPoint<false, kIntegerBits_, kFractionBits_>;

using fract8  = SignedFixedPoint<0,  7>;
using fract16 = SignedFixedPoint<0, 15>;
using fract32 = SignedFixedPoint<0, 31>;
using fract64 = SignedFixedPoint<0, 63>;

using ufract8  = UnsignedFixedPoint<0,  8>;
using ufract16 = UnsignedFixedPoint<0, 16>;
using ufract32 = UnsignedFixedPoint<0, 32>;
using ufract64 = UnsignedFixedPoint<0, 64>;

using accum8  = SignedFixedPoint< 3,  4>;
using accum16 = SignedFixedPoint< 7,  8>;
using accum32 = SignedFixedPoint<15, 16>;
using accum64 = SignedFixedPoint<31, 32>;

using uaccum8  = UnsignedFixedPoint< 4,  4>;
using uaccum16 = UnsignedFixedPoint< 8,  8>;
using uaccum32 = UnsignedFixedPoint<16, 16>;
using uaccum64 = UnsignedFixedPoint<32, 32>;


template<typename T>
struct IsFixedPoint : std::false_type {};

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits>
struct IsFixedPoint<FixedPoint<kSigned, kIntegerBits, kFractionBits>> : std::true_type {};
