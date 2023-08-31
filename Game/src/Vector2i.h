#pragma once

#include <stdint.h>

struct Vector2;

struct Vector2i
{
	int x;
	int y;

	Vector2i Add(Vector2i o) const;
    Vector2i AddValue(int value) const;
    Vector2i Subtract(Vector2i o) const;
    Vector2i SubtractValue(int value) const;
    Vector2i Scale(int value) const;
    Vector2i Multiply(Vector2i o) const;
    Vector2i Divide(Vector2i o) const;
    float Distance(Vector2i o) const;
    float SqrDistance(Vector2i o) const;
    Vector2i Negate() const;
    Vector2i Min(Vector2i min) const;
    Vector2i Max(Vector2i max) const;
};

constexpr static Vector2i Vec2i_ONE     = { 1, 1 };
constexpr static Vector2i Vec2i_ZERO    = { 0, 0 };
constexpr static Vector2i Vec2i_UP      = { 0, -1 };
constexpr static Vector2i Vec2i_DOWN    = { 0, 1 };
constexpr static Vector2i Vec2i_LEFT    = { -1, 0 };
constexpr static Vector2i Vec2i_RIGHT   = { 1, 0 };
constexpr static Vector2i Vec2i_NW      = { 0, -1 };
constexpr static Vector2i Vec2i_NE      = { 0, 1 };
constexpr static Vector2i Vec2i_SW      = { 1, 0 };
constexpr static Vector2i Vec2i_SE      = { -1, 0 };

constexpr static Vector2i Vec2i_MAX     = { INT32_MAX, INT32_MAX };
constexpr static Vector2i Vec2i_NULL    = Vec2i_MAX;

constexpr static Vector2i Vec2i_NEIGHTBORS[4] = { Vec2i_UP, Vec2i_RIGHT, Vec2i_DOWN, Vec2i_LEFT };
constexpr static Vector2i Vec2i_CORNERS[4] = { Vec2i_NW, Vec2i_NE, Vec2i_SW, Vec2i_SE };

constexpr static Vector2i Vec2i_NEIGHTBORS_CORNERS[8] = {
    Vec2i_NW,   Vec2i_UP ,  Vec2i_NE,
    Vec2i_LEFT,          Vec2i_RIGHT,
    Vec2i_SW,   Vec2i_DOWN, Vec2i_SE };

long long Vec2iPackInt64(Vector2i v);
Vector2i Vec2iUnpackInt64(long long packedI64);
Vector2 Vec2iToVec2(Vector2i v);
Vector2i Vec2ToVec2i(Vector2 v); // Floored

static inline bool operator==(Vector2i left, Vector2i right)
{
    return left.x == right.x && left.y == right.y;
}

static inline bool operator!=(Vector2i left, Vector2i right)
{
    return left.x != right.x || left.y != right.y;
}

static inline Vector2i operator+(Vector2i left, Vector2i right)
{
    return left.Add(right);
}

static inline Vector2i operator-(Vector2i left, Vector2i right)
{
    return left.Subtract(right);
}

static inline Vector2i operator*(Vector2i left, Vector2i right)
{
    return left.Multiply(right);
}

static inline Vector2i operator/(Vector2i left, Vector2i right)
{
    return left.Divide(right);
}
