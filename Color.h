#pragma once

#include "util.h"
#include "FixedPoint.h"
#include "ColorConstants.h"

struct Color : ColorConstants<Color> {
    
    using StorageT = accum32;

    StorageT r, g, b;

    constexpr Color() = default; 

    template<typename T>
    constexpr Color(T r, T g, T b) requires(std::is_integral<T>::value)
     : r(r, 256), g(g, 256), b(b, 256) {}

    template<typename T = StorageT>
    constexpr Color(T r, T g, T b): r(r), g(g), b(b) {}

    constexpr Color Inverse() const { return Color(1/r, 1/g, 1/b); }

    constexpr Color operator+(Color color) const { return Color(r+color.r, g+color.g, b+color.b); }
    constexpr Color operator-(Color color) const { return Color(r-color.r, g-color.g, b-color.b); }
    constexpr Color operator*(Color color) const { return Color(r*color.r, g*color.g, b*color.b); }
    constexpr Color operator/(Color color) const { return Color(r/color.r, g/color.g, b/color.b); }

    constexpr Color& operator+=(Color color)  { r+= color.r; g+= color.g; b+= color.b; return *this; }
    constexpr Color& operator-=(Color color)  { r-= color.r; g-= color.g; b-= color.b; return *this; }
    constexpr Color& operator*=(Color color)  { r*= color.r; g*= color.g; b*= color.b; return *this; }
    constexpr Color& operator/=(Color color)  { r/= color.r; g/= color.g; b/= color.b; return *this; }


    template<typename T> constexpr Color operator+(T scalar) const { return Color(r+scalar,  g+scalar,  b+scalar); }
    template<typename T> constexpr Color operator-(T scalar) const { return Color(r-scalar,  g-scalar,  b-scalar); }
    template<typename T> constexpr Color operator*(T scalar) const { return Color(r*scalar,  g*scalar,  b*scalar); }
    template<typename T> constexpr Color operator/(T scalar) const { return Color(r/scalar,  g/scalar,  b/scalar); }

    template<typename T> constexpr Color& operator+=(T scalar) { r+= scalar;  g+= scalar;  b+= scalar;  return *this; }
    template<typename T> constexpr Color& operator-=(T scalar) { r-= scalar;  g-= scalar;  b-= scalar;  return *this; }
    template<typename T> constexpr Color& operator*=(T scalar) { r*= scalar;  g*= scalar;  b*= scalar;  return *this; }
    template<typename T> constexpr Color& operator/=(T scalar) { r/= scalar;  g/= scalar;  b/= scalar;  return *this; }

    template<typename T> friend constexpr Color operator+(T scalar, Color color) { return Color(scalar+color.r,  scalar+color.g,  scalar+color.b); }
    template<typename T> friend constexpr Color operator-(T scalar, Color color) { return Color(scalar-color.r,  scalar-color.g,  scalar-color.b); }
    template<typename T> friend constexpr Color operator*(T scalar, Color color) { return Color(scalar*color.r,  scalar*color.g,  scalar*color.b); }
    template<typename T> friend constexpr Color operator/(T scalar, Color color) { return Color(scalar/color.r,  scalar/color.g,  scalar/color.b); }
};

template<bool kBigEndian_, uint8 kABits_, uint8 kRBits_, uint8 kGBits_, uint8 kBBits_>
struct ARGB {

    static inline constexpr bool kBigEndian = kBigEndian_;

    static inline constexpr uint8 kABits = kABits_; 
    static inline constexpr uint8 kRBits = kRBits_; 
    static inline constexpr uint8 kGBits = kGBits_; 
    static inline constexpr uint8 kBBits = kBBits_; 
    static inline constexpr uint8 kBits  = kABits + kRBits + kGBits + kBBits; 
    
    using AType = SpanningInteger<false, kABits>::type;
    using RType = SpanningInteger<false, kRBits>::type;
    using GType = SpanningInteger<false, kGBits>::type;
    using BType = SpanningInteger<false, kBBits>::type;
    using BitsT = SpanningInteger<false, kBits >::type;

    static inline constexpr uint8 kAShiftLE = kRBits + kGBits + kBBits;
    static inline constexpr uint8 kRShiftLE = kGBits + kBBits;
    static inline constexpr uint8 kGShiftLE = kBBits;
    static inline constexpr uint8 kBShiftLE = 0;

    static inline constexpr AType kMaxA = ~((~AType(0)) << kABits);
    static inline constexpr RType kMaxR = ~((~RType(0)) << kRBits);
    static inline constexpr GType kMaxG = ~((~GType(0)) << kGBits);
    static inline constexpr BType kMaxB = ~((~BType(0)) << kBBits);

    static inline constexpr BitsT kAMaskLE = BitsT(kMaxA) << kAShiftLE;
    static inline constexpr BitsT kRMaskLE = BitsT(kMaxR) << kRShiftLE;
    static inline constexpr BitsT kGMaskLE = BitsT(kMaxG) << kGShiftLE;
    static inline constexpr BitsT kBMaskLE = BitsT(kMaxB) << kBShiftLE;

    BitsT bits;

    constexpr operator BitsT() const { return bits; }

    constexpr BitsT GetBitsLE() const { return kBigEndian ? ByteSwap(bits) : bits; }

    constexpr void SetBitsLE(BitsT valueLE) { bits = kBigEndian ? ByteSwap(valueLE) : valueLE; }

    constexpr auto A() const { return (GetBitsLE() & kAMaskLE) >> kAShiftLE; }
    constexpr auto R() const { return (GetBitsLE() & kRMaskLE) >> kRShiftLE; }
    constexpr auto G() const { return (GetBitsLE() & kGMaskLE) >> kGShiftLE; }
    constexpr auto B() const { return (GetBitsLE() & kBMaskLE) >> kBShiftLE; }

    constexpr ARGB() = default;
    constexpr ARGB(BitsT bits): bits(bits) {}
    
    constexpr ARGB(Color color) {
        SetBitsLE(
            kAMaskLE |
            (BitsT(color.r.Fraction().bits >> (color.r.kFractionBits - kRBits)) << kRShiftLE) | 
            (BitsT(color.g.Fraction().bits >> (color.g.kFractionBits - kGBits)) << kGShiftLE) |
            (BitsT(color.b.Fraction().bits >> (color.b.kFractionBits - kBBits)) << kBShiftLE)
        ); 
    }

    constexpr ARGB(AType a, RType r, GType g, BType b) {
        SetBitsLE(
            ((BitsT(a) << kAShiftLE) & kAMaskLE) | 
            ((BitsT(r) << kRShiftLE) & kRMaskLE) | 
            ((BitsT(g) << kGShiftLE) & kGMaskLE) | 
            ((BitsT(b) << kBShiftLE) & kBMaskLE)
        );
    }

    constexpr operator Color() const { 
        return Color(
            Color::StorageT(R(), 1<<kRBits), 
            Color::StorageT(G(), 1<<kGBits), 
            Color::StorageT(B(), 1<<kBBits) 
        ); 
    }
};

using RGB16   = ARGB<false, 0, 5, 6, 5>;
using RGB16BE = ARGB<true,  0, 5, 6, 5>;

using RGB24   = ARGB<false, 0, 8, 8, 8>;
using RGB24BE = ARGB<true,  0, 8, 8, 8>;