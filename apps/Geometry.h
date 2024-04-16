#pragma once

#include <glib.h>


struct Point {
  explicit Point(gint x, gint y);

  Point operator+(const Point& other) const;
  Point operator-(const Point& other) const;
  Point operator/(const float& scalar) const;

  gint x;
  gint y;
};

class Vector {
  public:
    Vector();
    explicit Vector(Point end);
    explicit Vector(Point start, Point end);

    Point start;
    Point end;
    float magnitude;

    // Returns 2D cross product (determinant) of this and other point
    // treating other point as vector with same origin as this vector
    float cross(const Point& other) const;

  private:
    float calculate_magnitude() const;
};
