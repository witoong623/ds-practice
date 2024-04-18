#include "Analytic.h"


Analytic::Analytic() {}

void Analytic::draw_on_frame(NvDsBatchMeta *batch_meta) {
  // TODO: drawing based on configuration for each source
  NvDsFrameMeta *frame_meta = nvds_get_nth_frame_meta (batch_meta->frame_meta_list, 0);

  NvDsDisplayMeta *display_meta = nvds_acquire_display_meta_from_pool(batch_meta);
  NvOSD_LineParams *line_params = &display_meta->line_params[0];
  // coordinate is coordinate after frame is resized from streammux
  line_params->x1 = 100;
  line_params->y1 = 540;
  line_params->x2 = 1800;
  line_params->y2 = 540;
  line_params->line_width = 5;
  // default values are 0, which make it invisible due to alpha = 0
  line_params->line_color = (NvOSD_ColorParams){1.0, 0.0, 0.0, 1.0};
  display_meta->num_lines++;
  nvds_add_display_meta_to_frame(frame_meta, display_meta);
}
