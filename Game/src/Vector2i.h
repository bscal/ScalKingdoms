#pragma once

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
constexpr static Vector2i Vec2i_LEFT    = { 1, 0 };
constexpr static Vector2i Vec2i_RIGHT   = { -1, 0 };

constexpr static Vector2i Vec2i_NEIGHTBORS[4] = { Vec2i_UP, Vec2i_LEFT, Vec2i_DOWN, Vec2i_RIGHT };

constexpr static Vector2i Vec2i_NEIGHTBORS_CORNERS[8] = {
    Vec2i_UP, { 1, -1 }, Vec2i_LEFT, { 1, 1 },
    Vec2i_DOWN, { -1, 1 }, Vec2i_RIGHT, { -1, -1 } };

long long Vec2iPackInt64(Vector2i v);
Vector2i Vec2iUnpackInt64(long long packedI64);
Vector2 Vec2iToVec2(Vector2i v);
// Floored
Vector2i Vec2ToVec2i(Vector2 v);

static inline bool operator==(Vector2i left, Vector2i right)
{
    return left.x == right.x && left.y == right.y;
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
