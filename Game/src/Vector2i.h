#pragma once

#include "Base.h"

struct Vector2;

struct Vector2i
{
	int x;
	int y;

	_FORCE_INLINE_ Vector2i Add(Vector2i o) const;
    _FORCE_INLINE_ Vector2i AddValue(int value) const;
    _FORCE_INLINE_ Vector2i Subtract(Vector2i o) const;
    _FORCE_INLINE_ Vector2i SubtractValue(int value) const;
    _FORCE_INLINE_ Vector2i Scale(int value) const;
    _FORCE_INLINE_ Vector2i Multiply(Vector2i o) const;
    _FORCE_INLINE_ Vector2i Divide(Vector2i o) const;
    _FORCE_INLINE_ float Distance(Vector2i o) const;
    _FORCE_INLINE_ float SqrDistance(Vector2i o) const;
    _FORCE_INLINE_ int ManhattanDistance(Vector2i o) const; // Cost 1 per cardinal move, 2 per diagonal 
    _FORCE_INLINE_ int ManhattanDistanceWithCosts(Vector2i o) const; // Cost 10 per cardinal, 14 per diagonal
    _FORCE_INLINE_ Vector2i Negate() const;
    _FORCE_INLINE_ Vector2i VecMin(Vector2i min) const;
    _FORCE_INLINE_ Vector2i VecMax(Vector2i max) const;
};

constexpr static Vector2i Vec2i_ONE     = { 1, 1 };
constexpr static Vector2i Vec2i_ZERO    = { 0, 0 };
constexpr static Vector2i Vec2i_MAX     = { INT32_MAX, INT32_MAX };
constexpr static Vector2i Vec2i_NULL    = Vec2i_MAX;

constexpr static Vector2i Vec2i_UP      = { 0, -1 };
constexpr static Vector2i Vec2i_DOWN    = { 0, 1 };
constexpr static Vector2i Vec2i_LEFT    = { -1, 0 };
constexpr static Vector2i Vec2i_RIGHT   = { 1, 0 };
constexpr static Vector2i Vec2i_NW      = { -1, 1 };
constexpr static Vector2i Vec2i_NE      = { 1, 1 };
constexpr static Vector2i Vec2i_SW      = { -1, -1 };
constexpr static Vector2i Vec2i_SE      = { 1, -1 };

constexpr static Vector2i Vec2i_CARDINALS[4] = { Vec2i_UP, Vec2i_RIGHT, Vec2i_DOWN, Vec2i_LEFT };
constexpr static Vector2i Vec2i_DIAGONALS[4] = { Vec2i_NW, Vec2i_NE, Vec2i_SW, Vec2i_SE };

constexpr static Vector2i Vec2i_NEIGHTBORS[8] = {
    Vec2i_NW,   Vec2i_UP ,  Vec2i_NE,
    Vec2i_LEFT,          Vec2i_RIGHT,
    Vec2i_SW,   Vec2i_DOWN, Vec2i_SE };

_FORCE_INLINE_ i64 Vec2iPackInt64(Vector2i v);
_FORCE_INLINE_ Vector2i Vec2iUnpackInt64(i64 packedI64);
_FORCE_INLINE_ Vector2 Vec2iToVec2(Vector2i v);
_FORCE_INLINE_ Vector2i Vec2ToVec2i(Vector2 v); // Floored

i64 Vec2iPackInt64(Vector2i v)
{
    return ((i64)(v.x) << 32ll) | (i64)v.y;
}

Vector2i Vec2iUnpackInt64(i64 packedI64)
{
    Vector2i res;
    res.x = (int)(packedI64 >> 32ll);
    res.y = (int)packedI64;
    return res;
}

Vector2 Vec2iToVec2(Vector2i v)
{
    Vector2 res;
    res.x = (float)v.x;
    res.y = (float)v.y;
    return res;
}

Vector2i Vec2ToVec2i(Vector2 v)
{
    Vector2i res;
    res.x = (int)floorf(v.x);
    res.y = (int)floorf(v.y);
    return res;
}

Vector2i Vector2i::Add(Vector2i o) const
{
    return { x + o.x, y + o.y };
}

Vector2i Vector2i::AddValue(int add) const
{
    return { x + add, y + add };
}

Vector2i Vector2i::Subtract(Vector2i o) const
{
    return { x - o.x, y - o.y };
}

Vector2i Vector2i::SubtractValue(int sub) const
{
    return { x - sub, y - sub };
}

// Calculate distance between two vectors
float Vector2i::Distance(Vector2i o) const
{
    float result = sqrtf((float)((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y)));
    return result;
}

// Calculate square distance between two vectors
float Vector2i::SqrDistance(Vector2i o) const
{
    float result = (float)((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y));
    return result;
}

int Vector2i::ManhattanDistance(Vector2i o) const
{
	int xLength = abs(x - o.x);
	int yLength = abs(y - o.y);
	return Max(xLength, yLength);
}

int Vector2i::ManhattanDistanceWithCosts(Vector2i o) const
{
	constexpr int MOVE_COST = 10;
	constexpr int DIAGONAL_COST = 14;
	int xLength = abs(x - o.x);
	int yLength = abs(y - o.y);
	int res = MOVE_COST * (xLength + yLength) + (DIAGONAL_COST - 2 * MOVE_COST) * (int)fminf((float)xLength, (float)yLength);
	return res;
}

// Scale vector (multiply by value)
Vector2i Vector2i::Scale(int scale) const
{
    return { x * scale, y * scale };
}

// Multiply vector by vector
Vector2i Vector2i::Multiply(Vector2i o) const
{
    return { x * o.x, y * o.y };
}

// Negate vector
Vector2i Vector2i::Negate() const
{
    return { -x, -y };
}

Vector2i Vector2i::VecMin(Vector2i min) const
{
    Vector2i result;
    result.x = (x < min.x) ? min.x : x;
    result.y = (y < min.y) ? min.y : y;
    return result;
}

Vector2i Vector2i::VecMax(Vector2i max) const
{
    Vector2i result;
    result.x = (x > max.x) ? max.x : x;
    result.y = (y > max.y) ? max.y : y;
    return result;
}

// Divide vector by vector
Vector2i Vector2i::Divide(Vector2i o) const
{
    return { x / o.x, y / o.y };
}

static _FORCE_INLINE_ bool operator==(Vector2i left, Vector2i right)
{
    return left.x == right.x && left.y == right.y;
}

static _FORCE_INLINE_ bool operator!=(Vector2i left, Vector2i right)
{
    return left.x != right.x || left.y != right.y;
}

static _FORCE_INLINE_ Vector2i operator+=(Vector2i left, Vector2i right)
{
    return left.Add(right);
}

static _FORCE_INLINE_ Vector2i operator-=(Vector2i left, Vector2i right)
{
    return left.Subtract(right);
}

static _FORCE_INLINE_ Vector2i operator*=(Vector2i left, Vector2i right)
{
    return left.Multiply(right);
}

static _FORCE_INLINE_ Vector2i operator/=(Vector2i left, Vector2i right)
{
    return left.Divide(right);
}

static _FORCE_INLINE_ Vector2i operator+(Vector2i left, Vector2i right)
{
    return left.Add(right);
}

static _FORCE_INLINE_ Vector2i operator-(Vector2i left, Vector2i right)
{
    return left.Subtract(right);
}

static _FORCE_INLINE_ Vector2i operator*(Vector2i left, Vector2i right)
{
    return left.Multiply(right);
}

static _FORCE_INLINE_ Vector2i operator/(Vector2i left, Vector2i right)
{
    return left.Divide(right);
}
