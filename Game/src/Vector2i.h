#pragma once

#include <stdint.h>
#include <math.h>

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
    inline bool Equals(Vector2i other) const { return (x == other.x && y == other.y); }

    inline Vector2 AsVec2() const { return { (float)x, (float)y }; }
    static inline Vector2i FromVec2(Vector2 v) { return { (int)floorf(v.x), (int)floorf(v.y) }; }
};

inline const Vector2i Vec2i_ONE     = { 1, 1 };
inline const Vector2i Vec2i_ZERO    = { 0, 0 };
inline const Vector2i Vec2i_UP      = { 0, -1 };
inline const Vector2i Vec2i_DOWN    = { 0, 1 };
inline const Vector2i Vec2i_LEFT    = { 1, 0 };
inline const Vector2i Vec2i_RIGHT   = { -1, 0 };

inline const Vector2i Vec2i_NEIGHTBORS[4] = { Vec2i_UP, Vec2i_LEFT, Vec2i_DOWN, Vec2i_RIGHT };
inline const Vector2i Vec2i_NEIGHTBORS_CORNERS[8] = {
    Vec2i_UP, { 1, -1 }, Vec2i_LEFT, { 1, 1 },
    Vec2i_DOWN, { -1, 1 }, Vec2i_RIGHT, { -1, -1 } };

int64_t Vec2iPackInt64(Vector2i v);
Vector2i Vec2iUnpackInt64(int64_t packedVector);

static inline bool operator==(Vector2i left, Vector2i right)
{
    return left.Equals(right);
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

static inline Vector2i operator+=(Vector2i left, Vector2i right)
{
    return left = left.Add(right);
}

static inline Vector2i operator-=(Vector2i left, Vector2i right)
{
    return left = left.Subtract(right);
}

static inline Vector2i operator*=(Vector2i left, Vector2i right)
{
    return left = left.Multiply(right);
}

static inline Vector2i operator/=(Vector2i left, Vector2i right)
{
    return left = left.Divide(right);
}
