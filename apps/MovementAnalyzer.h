#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include <glib.h>

#include "Geometry.h"


// Which side the object is on the line based on cross product of 2 vectors
enum class SideOfLine {
  /* If object is outside the line limits or have not determine yet */
  Unknown,
  Left,
  Right,
};

enum class LineCrossDirection {
  // If object being outside line limits or travel out of line limits or have not determine yet
  Unknown,
  NotCross,
  LeftToRight,
  RightToLeft,
  ComeToLine
};

class ObjectHistory {
  public:
    explicit ObjectHistory(guint object_id, BoundingBox initial_bounding_box);

    // points on the object bounding box that represent body of the object
    // tell which side of the line ID the point is on
    std::unordered_map<guint, SideOfLine> side_on_lines;

    inline guint object_id() { return _object_id; }
    inline std::vector<Point> & anchor_points() { return _anchor_points; }
    void update_bounding_box(BoundingBox bbox);

    gint last_update_frame;
  private:
    guint _object_id;
    std::vector<Point> _anchor_points;
};


class Line {
  public:
    explicit Line(Point start, Point end);
    /* Update input object history and determine weather it crosses the line on which direction.
     * Return the direction of the object crossing the line immediately after it crossing.
     * Next update of object will not return cross status. */
    LineCrossDirection does_object_cross_line(ObjectHistory& object_history) const;
    gboolean is_points_on_line_limits(std::vector<Point> points) const;

    inline guint line_id() { return _line_id; }
  private:
    inline static guint _running_line_id = 0;

    guint _line_id;
    Point start;
    Point end;
    Vector vector;
    Vector start_limit;
    Vector end_limit;
};
