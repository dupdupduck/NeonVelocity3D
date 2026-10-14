#ifndef MATH3D_HPP
#define MATH3D_HPP

#include <cmath>
#include <cstring>
#include <algorithm>

namespace Math3D {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vector3 operator-(const Vector3& b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }

    Vector3& operator+=(const Vector3& b) { x += b.x; y += b.y; z += b.z; return *this; }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float Dot(const Vector3& b) const { return x * b.x + y * b.y + z * b.z; }

    Vector3 Cross(const Vector3& b) const {
        return {
            y * b.z - z * b.y,
            z * b.x - x * b.z,
            x * b.y - y * b.x
        };
    }

    float LengthSq() const { return x * x + y * y + z * z; }
    float Length() const { return std::sqrt(LengthSq()); }

    Vector3 Normalized() const {
        float l = Length();
        if (l > 1e-6f) return *this / l;
        return {0.0f, 0.0f, 0.0f};
    }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * std::clamp(t, 0.0f, 1.0f);
    }
};

struct Vector4 {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    Vector4() = default;
    Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};

struct Matrix4x4 {
    float m[16];

    Matrix4x4() { Identity(); }

    void Identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    const float* Data() const { return m; }

    Matrix4x4 operator*(const Matrix4x4& b) const {
        Matrix4x4 r;
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                r.m[col * 4 + row] =
                    m[0 * 4 + row] * b.m[col * 4 + 0] +
                    m[1 * 4 + row] * b.m[col * 4 + 1] +
                    m[2 * 4 + row] * b.m[col * 4 + 2] +
                    m[3 * 4 + row] * b.m[col * 4 + 3];
            }
        }
        return r;
    }

    static Matrix4x4 Translation(const Vector3& t) {
        Matrix4x4 r;
        r.m[12] = t.x;
        r.m[13] = t.y;
        r.m[14] = t.z;
        return r;
    }

    static Matrix4x4 Scale(const Vector3& s) {
        Matrix4x4 r;
        r.m[0]  = s.x;
        r.m[5]  = s.y;
        r.m[10] = s.z;
        return r;
    }

    static Matrix4x4 RotationX(float rad) {
        Matrix4x4 r;
        float c = std::cos(rad);
        float s = std::sin(rad);
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }

    static Matrix4x4 RotationY(float rad) {
        Matrix4x4 r;
        float c = std::cos(rad);
        float s = std::sin(rad);
        r.m[0] = c;  r.m[2] = -s;
        r.m[8] = s;  r.m[10] = c;
        return r;
    }

    static Matrix4x4 RotationZ(float rad) {
        Matrix4x4 r;
        float c = std::cos(rad);
        float s = std::sin(rad);
        r.m[0] = c;  r.m[1] = s;
        r.m[4] = -s; r.m[5] = c;
        return r;
    }

    static Matrix4x4 Perspective(float fovRad, float aspect, float nearZ, float farZ) {
        Matrix4x4 r;
        std::memset(r.m, 0, sizeof(r.m));
        float tanHalfFov = std::tan(fovRad * 0.5f);

        r.m[0]  = 1.0f / (aspect * tanHalfFov);
        r.m[5]  = 1.0f / tanHalfFov;
        r.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        r.m[11] = -1.0f;
        r.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return r;
    }

    static Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 f = (target - eye).Normalized();
        Vector3 r = f.Cross(up).Normalized();
        Vector3 u = r.Cross(f);

        Matrix4x4 m;
        m.m[0] = r.x;  m.m[4] = r.y;  m.m[8]  = r.z;  m.m[12] = -r.Dot(eye);
        m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;  m.m[13] = -u.Dot(eye);
        m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z; m.m[14] = f.Dot(eye);
        m.m[3] = 0.0f; m.m[7] = 0.0f; m.m[11] = 0.0f; m.m[15] = 1.0f;
        return m;
    }
};

} // namespace Math3D

#endif // MATH3D_HPP
