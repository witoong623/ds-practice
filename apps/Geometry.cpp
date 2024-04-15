#include "Geometry.h"

#include <glib.h>
#include <cmath>


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