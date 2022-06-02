#pragma once

#include "util.h"
#include "FixedPoint.h"
#include "ColorConstants.h"

struct Color : ColorConstants<Color> {
    
    using StorageT = accum32;
    
    StorageT r, g, b;

    //Note: Returns color in [R][G][B] uint32 order ([B][G][R] memory order) 
    constexpr uint16 RGB32() const { 

        return (uint32(r.Fraction().bits >> (r.kFractionBits - 8)) << 16) |  
               (uint32(g.Fraction().bits >> (g.kFractionBits - 8)) << 8 ) | 
               (uint32(b.Fraction().bits >> (b.kFractionBits - 8)) << 0 );
    };

    // Note: Returns color in [RRRRR:GGG][GGG:BBBBB]) uint16 order ([GGG:BBBBB][RRRRR:GGG] memory order)
    constexpr uint16 RGB16() const {

        return (uint16(r.Fraction().bits >> (r.kFractionBits - 5)) << 11) |  
               (uint16(g.Fraction().bits >> (g.kFractionBits - 6)) << 5 ) | 
               (uint16(b.Fraction().bits >> (b.kFractionBits - 5)) << 0 );
    };

    // Note: Returns color in [RRRRR:GGG][GGG:BBBBB]) memory order ([GGG:BBBBB][RRRRR:GGG] uint16 order)
    constexpr uint16 RGB16BE() const {

        uint16 rBits = r.Fraction().bits >> (r.kFractionBits - 5); 
        uint16 gBits = g.Fraction().bits >> (g.kFractionBits - 6); 
        uint16 bBits = b.Fraction().bits >> (b.kFractionBits - 5); 
        return (gBits << 13) |
               (bBits <<  8) |
               (rBits <<  3) |
               (gBits >>  3);
    };

    constexpr Color() = default; 

    template<typename T>
    constexpr Color(T r, T g, T b) requires(std::is_integral<T>::value)
     : r(r, 256), g(g, 256), b(b, 256) {}

    template<typename T = StorageT>
    constexpr Color(T r, T g, T b): r(r), g(g), b(b) {}

    static constexpr Color FromRGB16(uint16 rgb16) {
        uint8 r = rgb16 >> 11;
        uint8 g = (rgb16 >> 5) & 0x3F;
        uint8 b = rgb16 & 0x1F;

        return Color(StorageT(r, 32), 
                     StorageT(g, 64),
                     StorageT(b, 32));
    }

    static constexpr Color FromRGB16BE(uint16 rgb16BE) {
        uint8 r = (rgb16BE >> 3) & 0x1F;
        uint8 g = ((rgb16BE & 0x7) << 3) | (rgb16BE >> 13);
        uint8 b = (rgb16BE >> 8) & 0x1F;

        return Color(StorageT(r, 32), 
                     StorageT(g, 64),
                     StorageT(b, 32));
    }

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
