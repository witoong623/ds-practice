#include "MovementAnalyzer.h"

#include <algorithm>
#include <stdexcept>
#include <vector>


Line::Line(Point start, Point end) : start(start), end(end), vector(start, end),
  _line_id(_running_line_id++) {
  // Calculate the start and end limits
  float magnitude = vector.magnitude;

  if (magnitude == 0) {
    throw std::invalid_argument("Line must have non-zero length");
  }

  // Point can be vector start from the 0,0 origin
  Point delta_vec = end - start;
  Point unit_vec = delta_vec / magnitude;
  Point perpendicular_vec(-unit_vec.y, unit_vec.x);

  start_limit = Vector(vector.start, vector.start + perpendicular_vec);
  end_limit = Vector(vector.end, vector.end - perpendicular_vec);
}

gboolean Line::is_points_on_line_limits(std::vector<Point> points) const {
  return std::all_of(points.begin(), points.end(), [this](Point point) {
    return (start_limit.cross(point) > 0) == (end_limit.cross(point) > 0);
  });
}

LineCrossDirection Line::does_object_cross_line(ObjectHistory& obj_hist) const {
  // Check if the object is on the line limits
  gboolean is_on_line_limits = is_points_on_line_limits(obj_hist.anchor_points());

  if (!is_on_line_limits) {
    obj_hist.side_on_lines.erase(_line_id);
    return LineCrossDirection::Unknown;
  }

  std::vector<float> point_crosses(obj_hist.anchor_points().size());
  std::transform(obj_hist.anchor_points().begin(), obj_hist.anchor_points().end(), point_crosses.begin(),
    [this](Point point) {
      return vector.cross(point);
    }
  );

  gboolean all_on_left = std::all_of(point_crosses.begin(), point_crosses.end(),
                                     [](float cross) { return cross > 0; });
  gboolean all_on_right = std::all_of(point_crosses.begin(), point_crosses.end(),
                                      [](float cross) { return cross < 0; });
  if (all_on_left) {
    // first time seeing this object
    if (obj_hist.side_on_lines.find(_line_id) == obj_hist.side_on_lines.end()) {
      obj_hist.side_on_lines.insert({_line_id, SideOfLine::Left});
      return LineCrossDirection::NotCross;
    }

    SideOfLine previous_side = obj_hist.side_on_lines[_line_id];
    obj_hist.side_on_lines[_line_id] = SideOfLine::Left;

    if (previous_side == SideOfLine::Right) {
      return LineCrossDirection::RightToLeft;
    } else if (previous_side == SideOfLine::Left) {
      return LineCrossDirection::NotCross;
    } else {
      // previous side is unknown, but now on left, it just come in
      return LineCrossDirection::ComeToLine;
    }
  } else if (all_on_right) {
    // first time seeing this object
    if (obj_hist.side_on_lines.find(_line_id) == obj_hist.side_on_lines.end()) {
      obj_hist.side_on_lines.insert({_line_id, SideOfLine::Right});
      return LineCrossDirection::NotCross;
    }

    SideOfLine previous_side = obj_hist.side_on_lines[_line_id];
    obj_hist.side_on_lines[_line_id] = SideOfLine::Right;

    if (previous_side == SideOfLine::Right) {
      return LineCrossDirection::NotCross;
    } else if (previous_side == SideOfLine::Left) {
      return LineCrossDirection::LeftToRight;
    } else {
      // previous side is unknown, but now on left, it just come in
      return LineCrossDirection::ComeToLine;
    }
  } else {
    // object is on the line but it doesn't cross yet
    return LineCrossDirection::NotCross;
  }
}

ObjectHistory::ObjectHistory(guint object_id, BoundingBox initial_bounding_box):
  _object_id(object_id),
  _anchor_points(initial_bounding_box.get_anchor_points({AnchorPoint::TopLeft, AnchorPoint::TopRight,
                                                         AnchorPoint::BottomLeft, AnchorPoint::BottomRight})) {
}

void ObjectHistory::update_bounding_box(BoundingBox bbox) {
  auto anchor_points = bbox.get_anchor_points({AnchorPoint::TopLeft, AnchorPoint::TopRight,
                                               AnchorPoint::BottomLeft, AnchorPoint::BottomRight});
  _anchor_points = anchor_points;
}
