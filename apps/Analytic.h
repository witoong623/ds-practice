#pragma once

#include <vector>
#include <unordered_map>

#include <glib.h>
#include "gstnvdsmeta.h"

#include "MovementAnalyzer.h"


struct SourceAnalyticInfo {
  SourceAnalyticInfo(guint source_id) : source_id(source_id) {}

  guint source_id;
  gint last_update_frame;
  // object_id -> ObjectHistory
  std::unordered_map<guint64, ObjectHistory*> object_histories;
};

class Analytic {
  public:
    Analytic();

    void update_analytic_state(NvDsBatchMeta *batch_meta);
    // draw analytic lines, boxes, etc. on the frame
    void draw_on_frame(NvDsBatchMeta *batch_meta);
  private:
    void remove_stale_object_history(gint current_frame);

    // lines for crossing detection
    std::vector<Line> lines;
    std::unordered_map<guint, SourceAnalyticInfo*> source_analytic_infos;
};