#pragma once

#include "types.h"
#include "mathUtil.h"
#include "metaProgramming.h"

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits> 
struct FixedPoint;

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits> 
struct SaturatedFixedPoint;

template<typename T>
struct IsFixedPoint : std::false_type {};

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits>
struct IsFixedPoint<FixedPoint<kSigned, kIntegerBits, kFractionBits>> : std::true_type {};

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits>
struct IsFixedPoint<SaturatedFixedPoint<kSigned, kIntegerBits, kFractionBits>> : std::true_type {};

template<bool kSigned_, uint8 kIntegerBits_, uint8 kFractionBits_> 
struct FixedPoint {

    // TODO: add support for compile-time overflow detection

    static inline constexpr bool  kSigned       = kSigned_;
    static inline constexpr uint8 kIntegerBits  = kIntegerBits_;
    static inline constexpr uint8 kFractionBits = kFractionBits_;
    static inline constexpr uint8 kBits         = kSigned + kIntegerBits + kFractionBits;

    using BitsType     = SpanningInteger<kSigned, kBits>::type;
    using IntegerType  = SpanningInteger<kSigned, kIntegerBits>::type;
    using FractionType = FixedPoint<false, 0, kFractionBits>;

    using FloatBitShiftT = SpanningInteger<false, kFractionBits+1>::type;
    static inline constexpr FloatBitShiftT kFloatBitShift = FloatBitShiftT(1) << kFractionBits;

    BitsType bits : kBits;

    static inline constexpr FixedPoint FromBits(BitsType bits) {
        FixedPoint value;
        value.bits = bits;
        return value;
    }

    template<typename T>
    static inline constexpr auto ToBits(T n) {


        if constexpr(std::is_integral<T>::value) {

            using ResultT = SpanningInteger<std::is_signed<T>::value, Min(64u, 8*sizeof(T) + kFractionBits)>::type;
            return ResultT(n) << kFractionBits;

        } else if constexpr(std::is_floating_point<T>::value) {

            // TODO: make sure this doesn't overflow!
            return FixedPoint(n).bits;
            
        } else if constexpr(IsFixedPoint<T>::value) {
        
            if constexpr(n.kFractionBits > kFractionBits) {

                return n.bits >> (n.kFractionBits - kFractionBits);

            } else if constexpr(n.kFractionBits < kFractionBits) {

                constexpr uint8 kDeltaFractionBits = kFractionBits - n.kFractionBits;
                using ResultT = SpanningInteger<std::is_signed<T>::value, Min(64u, 8*sizeof(T) + kDeltaFractionBits)>::type;
            
                return ResultT(n.bits) << kDeltaFractionBits;
            
            } else {

                return n.bits;
            }
        
        } else {
            static_assert(Defer<T>(false), "Unsupported Type");
        }
    }

    template<typename T>
    static inline constexpr FixedPoint Saturated(T n) {
        
        constexpr T kTMinValue = kMinValue;
        constexpr T kTMaxValue = kMaxValue; 
 
        if(n <= kTMinValue) return kMinValue;
        if(n >= kTMaxValue) return kMaxValue;
        return n;
    }

    template<typename T1, typename T2>
    static inline constexpr auto MulBits(T1 bits1, T2 bits2) {

        // Note: (a*b)2^n = ((a2^b)(b2^n))/2^n

        constexpr bool kT1Signed = std::is_signed<T1>::value;
        constexpr bool kT2Signed = std::is_signed<T2>::value;

        constexpr uint8 kIntermediateBits = Min(64u, 8*(sizeof(T1)+sizeof(T2)) + (kT1Signed ^ kT2Signed) );
        constexpr bool kIntermediateSigned = kT1Signed || kT2Signed;

        using IntermediateT = SpanningInteger<kIntermediateSigned, kIntermediateBits>::type;

        if constexpr(sizeof(IntermediateT) == 8) {
 
            // TODO: optimize this?
            // Explicitly call Mul64 so gcc can inline code
            return Mul64(bits1, bits2) >> kFractionBits; 

        } else {
            return (IntermediateT(bits1) * IntermediateT(bits2)) >> kFractionBits;
        }
    }

    template<typename T1, typename T2>
    static inline constexpr auto DivBits(T1 bits1, T2 bits2) {
        
        // Note: (a/b)2^n = ((a2^b)2^n)/(b2^n)
        
        constexpr uint8 kIntermediateBits = Min(64u, 8*sizeof(T1) + kFractionBits);
        constexpr bool kIntermediateSigned = std::is_signed<T1>::value;

        using IntermediateT = SpanningInteger<kIntermediateSigned, kIntermediateBits>::type;
        
        return (IntermediateT(bits1) << kFractionBits) / bits2;
    }

    static inline constexpr FixedPoint kMinValue = FromBits(kSigned ? BitsType(1) << (kBits-1) : 0);
    
    //Note: Can't do ~BitsType(0) >> kSigned because bitsType can be signed
    static inline constexpr FixedPoint kMaxValue = []() {

        constexpr BitsType kOneBits = ~BitsType(0);
        constexpr uint8 kMaxBitShift = 8*sizeof(BitsType) - 1;

        constexpr uint8 kBitShift = kBits - kSigned;       
        BitsType kResultBits = (kBitShift <= kMaxBitShift) ? ~(kOneBits << kBitShift) : kOneBits;

        return FromBits(kResultBits); 
    }();

    constexpr IntegerType  Integer()  const { return bits >> kFractionBits; }
 
    constexpr FractionType Fraction() const { 

        // Note: this 'constexpr if' fixes warning message when 'BitsType(0) << kFractionBits' overflows
        if constexpr(kFractionBits == kBits) {
            return FractionType::FromBits(bits);
        } else {
            return FractionType::FromBits(bits & ~(~BitsType(0) << kFractionBits));
        }
    }

    constexpr FixedPoint() = default;

    template<bool kS, uint8 kI, uint8 kF>
    constexpr FixedPoint(FixedPoint<kS, kI, kF> v) {

        // // TODO: Throw warning here instead?
        // static_assert(kSigned >= kS,       "FixedPoint Construction Failed - constructed sign bits too small");
        // static_assert(kIntegerBits >= kI,  "FixedPoint Construction Failed - constructed integer bits too small");
        // static_assert(kFractionBits >= kF, "FixedPoint Construction Failed - constructed fraction bits too small");
    
        bits = ToBits(v);
    }

    template<typename T>
    constexpr FixedPoint(T n) requires(std::is_integral<T>::value) {

        if constexpr(sizeof(T) > sizeof(BitsType)) {
            bits = BitsType(n << kFractionBits);
        } else {
            bits = BitsType(n) << kFractionBits;
        }
    }

    template<typename T1, typename T2> 
    constexpr FixedPoint(T1 numerator, T2 denominator) {
        bits = BitsType( ToBits(numerator) / denominator );
    }


    // TODO: optimize with bitcast
    constexpr FixedPoint(double v) { bits = BitsType(v * kFloatBitShift); }
    constexpr FixedPoint(float  v) { bits = BitsType(v * kFloatBitShift); }

    template<typename T>
    constexpr explicit operator T() const requires(std::is_integral<T>::value) { return Integer(); }

    // TODO: optimize with bitcast
    constexpr explicit operator float()  const { return bits / float(kFloatBitShift); }
    constexpr explicit operator double() const { return bits / double(kFloatBitShift); }
    
    template<typename T> constexpr bool operator< (T n) const { return bits <  ToBits(n); }
    template<typename T> constexpr bool operator> (T n) const { return bits >  ToBits(n); }
    template<typename T> constexpr bool operator<=(T n) const { return bits <= ToBits(n); }
    template<typename T> constexpr bool operator>=(T n) const { return bits >= ToBits(n); }
    template<typename T> constexpr bool operator==(T n) const { return bits == ToBits(n); }
    template<typename T> constexpr bool operator!=(T n) const { return bits != ToBits(n); }

    template<typename T> friend constexpr bool operator< (T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) <  n; }
    template<typename T> friend constexpr bool operator> (T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) >  n; }
    template<typename T> friend constexpr bool operator<=(T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) <= n; }
    template<typename T> friend constexpr bool operator>=(T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) >= n; }
    template<typename T> friend constexpr bool operator==(T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) == n; }
    template<typename T> friend constexpr bool operator!=(T n, FixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return ToBits(n) != n; }

    // TODO: optimize integer multiply types? (No need to shift by power of 2) or does compiler already do that for us?
    
    template<typename T> constexpr FixedPoint& operator+=(T n) { bits+= ToBits(n); return *this; }
    template<typename T> constexpr FixedPoint& operator-=(T n) { bits-= ToBits(n); return *this; }
    template<typename T> constexpr FixedPoint& operator*=(T n) { bits = MulBits(bits, ToBits(n)); return *this; }
    template<typename T> constexpr FixedPoint& operator/=(T n) { bits = DivBits(bits, ToBits(n)); return *this; }

    template<typename T> constexpr FixedPoint operator+(T n) const { return FromBits(bits + ToBits(n)); }
    template<typename T> constexpr FixedPoint operator-(T n) const { return FromBits(bits - ToBits(n)); }
    template<typename T> constexpr FixedPoint operator*(T n) const { return FromBits(MulBits(bits, ToBits(n))); }
    template<typename T> constexpr FixedPoint operator/(T n) const { return FromBits(DivBits(bits, ToBits(n))); }

    template<typename T> friend constexpr FixedPoint operator+(T n, FixedPoint fixedPoint) { return FromBits(ToBits(n) + fixedPoint.bits); }
    template<typename T> friend constexpr FixedPoint operator-(T n, FixedPoint fixedPoint) { return FromBits(ToBits(n) - fixedPoint.bits); }
    template<typename T> friend constexpr FixedPoint operator*(T n, FixedPoint fixedPoint) { return FromBits(MulBits(ToBits(n), fixedPoint.bits)); }
    template<typename T> friend constexpr FixedPoint operator/(T n, FixedPoint fixedPoint) { return FromBits(DivBits(ToBits(n), fixedPoint.bits)); }

    template<typename T>
    constexpr FixedPoint SatAdd(T n) const {

        if constexpr(std::is_floating_point<T>::value) {

            return Saturated(T(*this) + n);

        } else {

            BitsType resultBits;
            auto nBits = ToBits(n);
            bool overflow = __builtin_add_overflow(bits, nBits, &resultBits);
            
            if(overflow) {
                if(n < 0) return kMinValue;
                return kMaxValue;
            }
            
            return FromBits(resultBits);
        }
    }

    template<typename T>
    constexpr FixedPoint SatSub(T n) const {

        if constexpr(std::is_floating_point<T>::value) {

            return Saturated(T(*this) - n);
        
        } else {

            BitsType resultBits;
            auto nBits = ToBits(n);
            bool overflow = __builtin_sub_overflow(bits, nBits, &resultBits);
            
            if(overflow) {
                if(n > 0) return kMinValue;
                return kMaxValue;
            }
            
            return FromBits(resultBits);
        }
    }

    template<typename T>
    constexpr FixedPoint SatMul(T n) const {

        if constexpr(std::is_floating_point<T>::value) {

            return Saturated(T(*this) * n);
        
        } else {

            auto nBits = ToBits(n);
            auto intermediateBits = MulBits(bits, nBits);

            BitsType resultBits = Clamp(intermediateBits, kMinValue.bits, kMaxValue.bits);
            return FromBits(resultBits);
        }
    }

    template<typename T>
    constexpr FixedPoint SatDiv(T n) const {

        if constexpr(std::is_floating_point<T>::value) {

            return Saturated(T(*this) * n);
        
        } else {

            auto nBits = ToBits(n);
            auto intermediateBits = DivBits(bits, nBits);

            BitsType resultBits = Clamp(intermediateBits, kMinValue.bits, kMaxValue.bits);
            return FromBits(resultBits);
        }
    }    
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

template<bool kSigned, uint8 kIntegerBits, uint8 kFractionBits> 
struct SaturatedFixedPoint: FixedPoint<kSigned, kIntegerBits, kFractionBits> {

    using BaseT = FixedPoint<kSigned, kIntegerBits, kFractionBits>;
    using BaseT::SatAdd;
    using BaseT::SatSub;
    using BaseT::SatMul;
    using BaseT::SatDiv;

    using BaseT::MulBits;
    using BaseT::DivBits;

    using BaseT::ToBits;
    using BaseT::Saturated;

    using BaseT::bits;
    using BaseT::kMaxValue;
    using BaseT::kMinValue;

    template<typename T>
    static inline constexpr SaturatedFixedPoint FromBits(T bits) {
        SaturatedFixedPoint result;
        result.bits = Clamp(bits, kMinValue.bits, kMaxValue.bits);
        return result;
    }    

    constexpr SaturatedFixedPoint() = default;

    constexpr SaturatedFixedPoint(BaseT fixedPoint) {
        bits = fixedPoint.bits;
    }

    template<typename T>
    constexpr SaturatedFixedPoint(T n) {
        bits = Saturated(n).bits;
    }

    template<typename T1, typename T2> 
    constexpr SaturatedFixedPoint(T1 numerator, T2 denominator) {

        auto intermediateBits = ToBits(numerator) / denominator;        
        bits = Clamp(intermediateBits, kMinValue.bits, kMaxValue.bits);
    }    

    template<typename T> constexpr SaturatedFixedPoint operator+(T n) const { return SatAdd(n); }
    template<typename T> constexpr SaturatedFixedPoint operator-(T n) const { return SatSub(n); }
    template<typename T> constexpr SaturatedFixedPoint operator*(T n) const { return SatMul(n); }
    template<typename T> constexpr SaturatedFixedPoint operator/(T n) const { return SatDiv(n); }

    template<typename T> constexpr SaturatedFixedPoint& operator+=(T n) { return *this = SatAdd(n); }
    template<typename T> constexpr SaturatedFixedPoint& operator-=(T n) { return *this = SatSub(n); }
    template<typename T> constexpr SaturatedFixedPoint& operator*=(T n) { return *this = SatMul(n); }
    template<typename T> constexpr SaturatedFixedPoint& operator/=(T n) { return *this = SatDiv(n); }

    template<typename T> friend constexpr SaturatedFixedPoint operator+(T n, SaturatedFixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return FromBits(ToBits(n) + fixedPoint.bits); }
    template<typename T> friend constexpr SaturatedFixedPoint operator-(T n, SaturatedFixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return FromBits(ToBits(n) - fixedPoint.bits); }
    template<typename T> friend constexpr SaturatedFixedPoint operator*(T n, SaturatedFixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return FromBits(MulBits(ToBits(n), fixedPoint.bits)); }
    template<typename T> friend constexpr SaturatedFixedPoint operator/(T n, SaturatedFixedPoint fixedPoint) requires(std::is_arithmetic<T>::value) { return FromBits(DivBits(ToBits(n), fixedPoint.bits)); }  
};

template<uint8 kIntegerBits_, uint8 kFractionBits_>
using SignedSaturatedFixedPoint = SaturatedFixedPoint<true, kIntegerBits_, kFractionBits_>;

template<uint8 kIntegerBits_, uint8 kFractionBits_>
using UnsignedSaturatedFixedPoint = SaturatedFixedPoint<false, kIntegerBits_, kFractionBits_>;


using satfract8  = SignedSaturatedFixedPoint<0,  7>;
using satfract16 = SignedSaturatedFixedPoint<0, 15>;
using satfract32 = SignedSaturatedFixedPoint<0, 31>;
using satfract64 = SignedSaturatedFixedPoint<0, 63>;

using usatfract8  = UnsignedSaturatedFixedPoint<0,  8>;
using usatfract16 = UnsignedSaturatedFixedPoint<0, 16>;
using usatfract32 = UnsignedSaturatedFixedPoint<0, 32>;
using usatfract64 = UnsignedSaturatedFixedPoint<0, 64>;

using sat8  = SignedSaturatedFixedPoint< 3,  4>;
using sat16 = SignedSaturatedFixedPoint< 7,  8>;
using sat32 = SignedSaturatedFixedPoint<15, 16>;
using sat64 = SignedSaturatedFixedPoint<31, 32>;

using usat8  = UnsignedSaturatedFixedPoint< 4,  4>;
using usat16 = UnsignedSaturatedFixedPoint< 8,  8>;
using usat32 = UnsignedSaturatedFixedPoint<16, 16>;
using usat64 = UnsignedSaturatedFixedPoint<32, 32>;
