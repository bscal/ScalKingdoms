#include "Vector2i.h"

#include <math.h>

struct Vector2
{
    float x;
    float y;
};

long long Vec2iPackInt64(Vector2i v)
{
    return (long long)v.x << 32ll | v.y;
}

Vector2i Vec2iUnpackInt64(long long packedI64)
{
    int y = (int)packedI64;
    int x = (int)(packedI64 >> 32);
    return { x, y };
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

Vector2i Vector2i::Min(Vector2i min) const
{
    Vector2i result;
    result.x = (x < min.x) ? min.x : x;
    result.y = (y < min.y) ? min.y : y;
    return result;
}

Vector2i Vector2i::Max(Vector2i max) const
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
