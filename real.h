#pragma once

// TODO: make this 32 and 64 bit compatible
struct Real32 {

    using ValueType = float;
    using IntegerType = int32;

    using FractionType = float;

    // // TODO: Hack to test Color
    // static inline constexpr uint8 kFractionBits = 32;
    // using FractionType = UnsignedFixedPoint<0, kFractionBits>;

    // TODO: Add min, max, infity, etc...

    ValueType value;

    constexpr IntegerType  Integer()  const { return value; }
    constexpr FractionType Fraction() const { return value - IntegerType(value); }

    constexpr Real32() = default;

    template<typename T> 
    constexpr Real32(T n) { value = ValueType(n); }

    template<typename T1, typename T2> 
    constexpr Real32(T1 numerator, T2 denominator) { 

        // TODO: Make sure numerator can fit in ValueType otherwise use double
        value = ValueType(numerator) / denominator; 
    }

    template<typename T>
    constexpr explicit operator T() const { return Integer(); }

    constexpr operator float()  const { return float(value); }
    constexpr explicit operator double() const { return double(value); }
    
    constexpr bool operator<(Real32 v) const { return value < v.value; }
    constexpr bool operator>(Real32 v) const { return value > v.value; }

    constexpr bool operator<=(Real32 v) const { return value <= v.value; }
    constexpr bool operator>=(Real32 v) const { return value >= v.value; }    

    constexpr bool operator==(Real32 v) const { return value == v.value; }
    constexpr bool operator!=(Real32 v) const { return value != v.value; }

    constexpr Real32 operator+(Real32 n) const { return value + n.value; }
    constexpr Real32 operator-(Real32 n) const { return value - n.value; }
    constexpr Real32 operator*(Real32 n) const { return value * n.value; }
    constexpr Real32 operator/(Real32 n) const { return value / n.value; }

    constexpr Real32& operator+=(Real32 n) { value+= n.value; return *this; }
    constexpr Real32& operator-=(Real32 n) { value-= n.value; return *this; }
    constexpr Real32& operator*=(Real32 n) { value*= n.value; return *this; }
    constexpr Real32& operator/=(Real32 n) { value/= n.value; return *this; }

    template<typename T> friend constexpr Real32 operator+(T n, Real32 real32) requires(std::is_arithmetic<T>::value) { return real32.operator+(n); }
    template<typename T> friend constexpr Real32 operator-(T n, Real32 real32) requires(std::is_arithmetic<T>::value) { return Real32(n) - real32; }
    template<typename T> friend constexpr Real32 operator*(T n, Real32 real32) requires(std::is_arithmetic<T>::value) { return real32.operator*(n); }
    template<typename T> friend constexpr Real32 operator/(T n, Real32 real32) requires(std::is_arithmetic<T>::value) { return Real32(n) / real32; }  
};