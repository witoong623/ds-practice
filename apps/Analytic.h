#pragma once

#include <vector>

#include <glib.h>
#include "gstnvdsmeta.h"

#include "MovementAnalyzer.h"


class Analytic {
  public:
    Analytic();

    void update_analytic_state(NvDsBatchMeta *batch_meta);
    // draw analytic lines, boxes, etc. on the frame
    void draw_on_frame(NvDsBatchMeta *batch_meta);
  private:
    // lines for crossing detection
    std::vector<Line> lines;
};