#pragma once

#include "util.h"
#include "FixedPoint.h"

template<typename ColorT>
struct ColorConstants {

    static inline constexpr auto kMin = ColorT::StorageT::kMinValue;
    static inline constexpr auto kMax = ColorT::StorageT::kMaxValue;

    static inline constexpr ColorT kRed     = ColorT(kMax, kMin, kMin);
    static inline constexpr ColorT kGreen   = ColorT(kMin, kMax, kMin);
    static inline constexpr ColorT kBlue    = ColorT(kMin, kMin, kMax);
    static inline constexpr ColorT kCyan    = ColorT(kMin, kMax, kMax);
    static inline constexpr ColorT kYellow  = ColorT(kMax, kMax, kMin);
    static inline constexpr ColorT kMagenta = ColorT(kMax, kMin, kMax);
    static inline constexpr ColorT kBlack   = ColorT(kMin, kMin, kMin);
    static inline constexpr ColorT kWhite   = ColorT(kMax, kMax, kMax);
};

struct Color : ColorConstants<Color> {
    
    using StorageT = ufract32;

    StorageT r, g, b;

    //Note: Returns color in [R][G][B] uint32 order ([B][G][R] memory order) 
    constexpr uint16 RGB32() const { 
        return (uint32(r.Fraction() >> (r.kFractionBits - 8)) << 16) |  
               (uint32(g.Fraction() >> (g.kFractionBits - 8)) << 8 ) | 
               (uint32(b.Fraction() >> (b.kFractionBits - 8)) << 0 );
    };

    // Note: Returns color in [RRRRR:GGG][GGG:BBBBB]) uint16 order ([GGG:BBBBB][RRRRR:GGG] memory order)
    constexpr uint16 RGB16() const {
        return (uint16(r.Fraction() >> (r.kFractionBits - 5)) << 11) |  
               (uint16(g.Fraction() >> (g.kFractionBits - 6)) << 5 ) | 
               (uint16(b.Fraction() >> (b.kFractionBits - 5)) << 0 );
    };

    // Note: Returns color in [RRRRR:GGG][GGG:BBBBB]) memory order ([GGG:BBBBB][RRRRR:GGG] uint16 order)
    constexpr uint16 RGB16BE() const {

        uint16 rBits = r.Fraction() >> (r.kFractionBits - 5); 
        uint16 gBits = g.Fraction() >> (g.kFractionBits - 6); 
        uint16 bBits = b.Fraction() >> (b.kFractionBits - 5); 
        return (gBits << 13) |
               (bBits <<  8) |
               (rBits <<  3) |
               (gBits >>  3);
    };

    constexpr Color(StorageT r = 0, StorageT g = 0, StorageT b = 0): r(r), g(g), b(b) {}

    constexpr Color Inverse() const { return Color(1/r, 1/g, 1/b); }

    // TODO: swap out float for accum type that saturates!!!!

    constexpr Color operator+(float scalar) const { return Color(r+scalar,  g+scalar,  b+scalar); }
    constexpr Color operator-(float scalar) const { return Color(r-scalar,  g-scalar,  b-scalar); }
    constexpr Color operator*(float scalar) const { return Color(r*scalar,  g*scalar,  b*scalar); }
    constexpr Color operator/(float scalar) const { return Color(r/scalar,  g/scalar,  b/scalar); }

    constexpr Color operator+(Color color) const { return Color(r+color.r, g+color.g, b+color.b); }
    constexpr Color operator-(Color color) const { return Color(r-color.r, g-color.g, b-color.b); }
    constexpr Color operator*(Color color) const { return Color(r*color.r, g*color.g, b*color.b); }
    constexpr Color operator/(Color color) const { return Color(r/color.r, g/color.g, b/color.b); }

    constexpr Color& operator+=(Color color)  { r+= color.r; g+= color.g; b+= color.b; return *this; }
    constexpr Color& operator-=(Color color)  { r-= color.r; g-= color.g; b-= color.b; return *this; }
    constexpr Color& operator*=(Color color)  { r*= color.r; g*= color.g; b*= color.b; return *this; }
    constexpr Color& operator/=(Color color)  { r/= color.r; g/= color.g; b/= color.b; return *this; }

    constexpr Color& operator+=(float scalar) { r+= scalar;  g+= scalar;  b+= scalar;  return *this; }
    constexpr Color& operator-=(float scalar) { r-= scalar;  g-= scalar;  b-= scalar;  return *this; }
    constexpr Color& operator*=(float scalar) { r*= scalar;  g*= scalar;  b*= scalar;  return *this; }
    constexpr Color& operator/=(float scalar) { r/= scalar;  g/= scalar;  b/= scalar;  return *this; }

    friend constexpr Color operator+(float scalar, Color color) { return Color(scalar+color.r,  scalar+color.g,  scalar+color.b); }
    friend constexpr Color operator-(float scalar, Color color) { return Color(scalar-color.r,  scalar-color.g,  scalar-color.b); }
    friend constexpr Color operator*(float scalar, Color color) { return Color(scalar*color.r,  scalar*color.g,  scalar*color.b); }
    friend constexpr Color operator/(float scalar, Color color) { return scalar * color.Inverse(); }
};
