#pragma once

#include <cmath>
#include <SFML/System.hpp>

template <typename T>
class Vec2 {
public:
    T x, y;

    Vec2() : x(0), y(0) {}
    Vec2(T xin, T yin) : x(xin), y(yin) {}
    Vec2(const sf::Vector2<T>& vec) : x(vec.x), y(vec.y) {}

    // Convert to SFML Vector2
    operator sf::Vector2<T>() const { return sf::Vector2<T>(x, y); }

    // Basic vector operations
    Vec2 operator+(const Vec2& rhs) const { return Vec2(x + rhs.x, y + rhs.y); }
    Vec2 operator-(const Vec2& rhs) const { return Vec2(x - rhs.x, y - rhs.y); }
    Vec2 operator*(T val) const { return Vec2(x * val, y * val); }
    Vec2 operator/(T val) const { 
        return (val != 0) ? Vec2(x / val, y / val) : Vec2(0, 0);
    }

    // Compound operations
    Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    Vec2& operator*=(T val) { x *= val; y *= val; return *this; }
    Vec2& operator/=(T val) { 
        if (val != 0) { x /= val; y /= val; } 
        return *this;
    }

    // Vector calculations
    T dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }
    T magnitude() const { return std::sqrt(x * x + y * y); }
    
    Vec2 normalize() const { 
        T mag = magnitude();
        return (mag > 0) ? (*this / mag) : Vec2(0, 0);
    }

    T distance(const Vec2& rhs) const {
        return (*this - rhs).magnitude();
    }

    // Comparisons
    bool operator==(const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }
    bool operator!=(const Vec2& rhs) const { return !(*this == rhs); }
    bool isZero() const { return x == 0 && y == 0; }
};

using Vec2f = Vec2<float>;
