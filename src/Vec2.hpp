#pragma once

#include <cmath>          // For sqrt
#include <SFML/System.hpp> // For sf::Vector2

// Template-based 2D vector class
template <typename T>
class Vec2 {
public:
    T x, y;  // Coordinates

    // Constructors
    Vec2() : x(0), y(0) {} // Default to (0,0)
    Vec2(T xin, T yin) : x(xin), y(yin) {}
    Vec2(const sf::Vector2<T>& vec) : x(vec.x), y(vec.y) {}

    // Implicit conversion to SFML's Vector2
    operator sf::Vector2<T>() const { return sf::Vector2<T>(x, y); }

    // Arithmetic operators
    Vec2 operator+(const Vec2& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
    Vec2 operator-(const Vec2& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
    Vec2 operator*(T val) const { return Vec2(x * val, y * val); }
    Vec2 operator/(T val) const { return (val != 0) ? Vec2(x / val, y / val) : Vec2(0, 0); } // Avoid division by zero

    // Compound assignment operators
    Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vec2& operator*=(T val) { x *= val; y *= val; return *this; }
    Vec2& operator/=(T val) { if (val != 0) { x /= val; y /= val; } return *this; } // Avoid division by zero

    // Dot product
    T dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }

    // Magnitude (length)
    T magnitude() const { return std::sqrt(x * x + y * y); }

    // Normalize (returns zero vector if length is zero)
    Vec2 normalize() const { 
        T mag = magnitude();
        return (mag > 0) ? (*this / mag) : Vec2(0, 0);
    }

    // Distance between two points
    T distance(const Vec2& rhs) const {
        return std::sqrt((x - rhs.x) * (x - rhs.x) + (y - rhs.y) * (y - rhs.y));
    }

    // Equality operators
    bool operator==(const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const Vec2& rhs) const { return !(*this == rhs); }

    // Check if the vector is zero
    bool isZero() const { return x == 0 && y == 0; }
};

// Typedef for convenience
using Vec2f = Vec2<float>;
