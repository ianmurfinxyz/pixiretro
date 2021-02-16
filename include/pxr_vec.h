#ifndef _PIXIRETRO_MATH_VEC_H_
#define _PIXIRETRO_MATH_VEC_H_

#include <cmath>

namespace pxr
{

struct Vector2f;

struct Vector2i
{
  constexpr Vector2i() : _x{0}, _y{0} {}
  constexpr Vector2i(int32_t x, int32_t y) : _x{x}, _y{y} {}
  constexpr Vector2i(const Vector2f& v);

  Vector2i(const Vector2i&) = default;
  Vector2i(Vector2i&&) = default;
  Vector2i& operator=(const Vector2i&) = default;
  Vector2i& operator=(Vector2i&&) = default;

  void zero() {_x = _y = 0;}
  bool isZero() {return _x == 0 && _y == 0;}
  Vector2i operator+(const Vector2i& v) const {return Vector2i{_x + v._x, _y + v._y};}
  void operator+=(const Vector2i& v) {_x += v._x; _y += v._y;}
  Vector2i operator-(const Vector2i& v) const {return Vector2i{_x - v._x, _y - v._y};}
  void operator-=(const Vector2i& v) {_x -= v._x; _y -= v._y;}
  Vector2i operator*(float scale) const {return Vector2i(_x * scale, _y * scale);}
  void operator*=(float scale) {_x *= scale; _y *= scale;}
  void operator*=(int32_t scale) {_x *= scale; _y *= scale;}
  float dot(const Vector2i& v) {return (_x * v._x) + (_y * v._y);}
  float cross(const Vector2i& v) const {return (_x * v._y) - (_y * v._x);}
  float length() const {return std::hypot(_x, _y);}
  float lengthSquared() const {return (_x * _x) + (_y * _y);}
  inline Vector2i normalized() const;
  inline void normalize();

  bool operator==(const Vector2i& v) const {return _x == v._x && _y == v._y;}
  bool operator!=(const Vector2i& v) const {return _x != v._x && _y != v._y;}

  int32_t _x;
  int32_t _y;
};

struct Vector2f
{
  constexpr Vector2f() : _x{0.f}, _y{0.f} {}
  constexpr Vector2f(float x, float y) : _x{x}, _y{y} {}
  constexpr Vector2f(const Vector2i& v);

  Vector2f(const Vector2f&) = default;
  Vector2f(Vector2f&&) = default;
  Vector2f& operator=(const Vector2f&) = default;
  Vector2f& operator=(Vector2f&&) = default;

  void zero() {_x = _y = 0.f;}
  bool isZero() {return _x == 0.f && _y == 0.f;}
  Vector2f operator+(const Vector2f& v) const {return Vector2f{_x + v._x, _y + v._y};}
  void operator+=(const Vector2f& v) {_x += v._x; _y += v._y;}
  Vector2f operator-(const Vector2f& v) const {return Vector2f{_x - v._x, _y - v._y};}
  void operator-=(const Vector2f& v) {_x -= v._x; _y -= v._y;}
  Vector2f operator*(float scale) const {return Vector2f{_x * scale, _y * scale};}
  void operator*=(float scale) {_x *= scale; _y *= scale;}
  void operator*=(int32_t scale) {_x *= scale; _y *= scale;}
  float dot(const Vector2f& v) {return (_x * v._x) + (_y * v._y);}
  float cross(const Vector2f& v) const {return (_x * v._y) - (_y * v._x);}
  float length() const {return std::hypot(_x, _y);}
  float lengthSquared() const {return (_x * _x) + (_y * _y);}
  inline Vector2f normalized() const;
  inline void normalize();

  float _x;
  float _y;
};

constexpr Vector2i::Vector2i(const Vector2f& v) :
  _x{static_cast<int32_t>(v._x)},
  _y{static_cast<int32_t>(v._y)}
{}

Vector2i Vector2i::normalized() const
{
  Vector2i v = *this;
  v.normalize();
  return v;
}

void Vector2i::normalize()
{
  float l = (_x * _x) + (_y * _y);
  if(l) {
    l = std::sqrt(l);
    _x /= l;
    _y /= l;
  }
}

constexpr Vector2f::Vector2f(const Vector2i& v) :
  _x{static_cast<float>(v._x)},
  _y{static_cast<float>(v._y)}
{}

Vector2f Vector2f::normalized() const
{
  Vector2f v = *this;
  v.normalize();
  return v;
}

void Vector2f::normalize()
{
  float l = (_x * _x) + (_y * _y);
  if(l) {
    l = std::sqrt(l);
    _x /= l;
    _y /= l;
  }
}

} // namespace pxr

#endif
