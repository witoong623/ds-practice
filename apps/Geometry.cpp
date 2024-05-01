#include "Geometry.h"

#include <cmath>
#include <vector>

#include <glib.h>


Point::Point(gint x, gint y) : x(x), y(y) {}

Point Point::operator+(const Point& other) const {
  return Point(x + other.x, y + other.y);
}

Point Point::operator-(const Point& other) const {
  return Point(x - other.x, y - other.y);
}

Point Point::operator/(const float& scalar) const {
  return Point(x / scalar, y / scalar);
}

Vector::Vector() : start(0, 0), end(1, 0), magnitude(this->calculate_magnitude()) {}

Vector::Vector(Point end) : start(0, 0), end(end), magnitude(this->calculate_magnitude()) {}

Vector::Vector(Point start, Point end) : start(start), end(end), magnitude(this->calculate_magnitude()) {}

float Vector::cross(const Point &other) const {
  return (end.x - start.x) * (other.y - start.y) -
         (end.y - start.y) * (other.x - start.x);
}

float Vector::calculate_magnitude() const {
  return std::sqrt(std::pow(end.x - start.x, 2) + std::pow(end.y - start.y, 2));
}

BoundingBox::BoundingBox(Point top_left, Point bottom_right):
  top_left(top_left), bottom_right(bottom_right) {}

std::vector<Point> BoundingBox::get_anchor_points(std::initializer_list<AnchorPoint> anchor_points) const {
  std::vector<Point> points;
  for (auto anchor_point : anchor_points) {
    switch (anchor_point) {
      case AnchorPoint::TopLeft:
        points.push_back(top_left);
        break;
      case AnchorPoint::TopRight:
        points.push_back(Point(bottom_right.x, top_left.y));
        break;
      case AnchorPoint::BottomLeft:
        points.push_back(Point(top_left.x, bottom_right.y));
        break;
      case AnchorPoint::BottomRight:
        points.push_back(bottom_right);
        break;
    }
  }
  return points;
}