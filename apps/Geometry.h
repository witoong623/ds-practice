#pragma once

#include <initializer_list>
#include <vector>

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


enum class AnchorPoint {
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
};

class BoundingBox {
  public:
    explicit BoundingBox(Point top_left, Point bottom_right);

    std::vector<Point> get_anchor_points(std::initializer_list<AnchorPoint> anchor_points) const;

    Point top_left;
    Point bottom_right;
};
