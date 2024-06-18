#pragma once

#include <vector>
#include <unordered_map>

#include <glib.h>
#include "gstnvdsmeta.h"

#include "FrameBuffer.h"
#include "MovementAnalyzer.h"


struct SourceAnalyticInfo {
  SourceAnalyticInfo(guint source_id) : source_id(source_id) {}

  guint source_id;
  gint last_update_frame;
  // object_id -> ObjectHistory
  std::unordered_map<guint64, ObjectHistory*> object_histories;
};

typedef std::unordered_map<guint, SourceAnalyticInfo*> SourceAnalyticInfoMap;

// Information for doing line crossing analysis for a single line
struct LineCrossing {
  guint source_id;
  Line line;
  // direction that count as crossing the line
  std::vector<LineCrossDirection> count_directions;
  std::unordered_map<LineCrossDirection, guint64> crossing_direction_counts;
};

class Analytic {
  public:
    Analytic(FrameBuffer *frame_buffer);

    // update all states of analytics using object in current frame
    void update_analytic_state(NvDsBatchMeta *batch_meta);
    // draw analytic's states i.e. lines, boxes, etc. on the frame
    void draw_on_frame(NvDsBatchMeta *batch_meta);
  private:
    void remove_stale_object_history(gint current_frame);
    void update_line_crossing_analysis(gint current_frame);

    FrameBuffer *frame_buffer;
    SourceAnalyticInfoMap source_analytic_infos;

    // source_id -> line crossing check info
    std::unordered_map<guint, std::vector<LineCrossing>> line_crossing_infos;
};