#pragma once
#include <cmath>
#include <cstdint>

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
    
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float dist_to(const Vec3& v) const { return (*this - v).length(); }
    
    bool is_zero() const { return x == 0.f && y == 0.f && z == 0.f; }
};

struct Vec2 {
    float x, y;
    Vec2() : x(0), y(0) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct ViewMatrix {
    float m[4][4];
};

namespace Math {
    bool world_to_screen(const Vec3& world, Vec2& screen, const ViewMatrix& view, int width, int height);
}
