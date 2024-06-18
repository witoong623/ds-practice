#include "Analytic.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "gstnvdsmeta.h"
#include <opencv2/opencv.hpp>

#include "FrameBuffer.h"
#include "Geometry.h"
#include "MovementAnalyzer.h"

constexpr gint STALE_OBJECT_THRESHOLD = 250;
constexpr guint FIRST_SOURCE = 0;
constexpr guint MAX_DISPLAY_LEN = 64;

Analytic::Analytic(FrameBuffer *frame_buffer): frame_buffer(frame_buffer) {
  // TODO: create line from configuration
  Line line {Point(100, 440), Point(1800, 440)};
  // create LineCrossing
  line_crossing_infos[FIRST_SOURCE].push_back({
    FIRST_SOURCE,
    line,
    {LineCrossDirection::RightToLeft, LineCrossDirection::LeftToRight}
  });
}

void Analytic::draw_on_frame(NvDsBatchMeta *batch_meta) {
  // TODO: drawing based on configuration for each source
  NvDsFrameMeta *frame_meta = nvds_get_nth_frame_meta (batch_meta->frame_meta_list, 0);

  NvDsDisplayMeta *display_meta = nvds_acquire_display_meta_from_pool(batch_meta);

  // drawing configuration
  NvOSD_LineParams *line_params = &display_meta->line_params[0];
  // coordinate is coordinate after frame is resized from streammux
  line_params->x1 = 100;
  line_params->y1 = 440;
  line_params->x2 = 1800;
  line_params->y2 = 440;
  line_params->line_width = 5;
  // default values are 0, which make it invisible due to alpha = 0
  line_params->line_color = (NvOSD_ColorParams){1.0, 0.0, 0.0, 1.0};
  display_meta->num_lines++;

  // drawing result of analytics
  NvOSD_TextParams *text_params = &display_meta->text_params[0];
  display_meta->num_labels++;
  text_params->display_text = static_cast<char*>(g_malloc0(MAX_DISPLAY_LEN));
  std::snprintf(text_params->display_text, MAX_DISPLAY_LEN, "In: %lu, Out: %lu",
                line_crossing_infos[FIRST_SOURCE][0].crossing_direction_counts[LineCrossDirection::RightToLeft],
                line_crossing_infos[FIRST_SOURCE][0].crossing_direction_counts[LineCrossDirection::LeftToRight]);

  text_params->x_offset = 100;
  text_params->y_offset = 440;

  text_params->font_params.font_name = "Serif";
  text_params->font_params.font_size = 12;
  text_params->font_params.font_color.red = 1.0;
  text_params->font_params.font_color.green = 1.0;
  text_params->font_params.font_color.blue = 1.0;
  text_params->font_params.font_color.alpha = 1.0;

  text_params->set_bg_clr = TRUE;
  text_params->text_bg_clr.red = 0.0;
  text_params->text_bg_clr.green = 0.0;
  text_params->text_bg_clr.blue = 0.0;
  text_params->text_bg_clr.alpha = 1.0;

  nvds_add_display_meta_to_frame(frame_meta, display_meta);
}

void Analytic::update_analytic_state(NvDsBatchMeta *batch_meta) {
  NvDsFrameMeta *frame_meta = nvds_get_nth_frame_meta (batch_meta->frame_meta_list, 0);
  NvDsObjectMetaList *obj_meta_list = frame_meta->obj_meta_list;

  SourceAnalyticInfo *source_info = source_analytic_infos[frame_meta->source_id];
  if (source_info == nullptr) {
    source_analytic_infos[frame_meta->source_id] = new SourceAnalyticInfo(frame_meta->source_id);
  }

  if (obj_meta_list != nullptr) {
    source_info->last_update_frame = frame_meta->frame_num;
  }

  for (; obj_meta_list != nullptr; obj_meta_list = obj_meta_list->next) {
    NvDsObjectMeta *obj_meta = static_cast<NvDsObjectMeta*>(obj_meta_list->data);

    auto obj_bbox = &obj_meta->detector_bbox_info.org_bbox_coords;
    ObjectHistory * history = source_info->object_histories[obj_meta->object_id];
    if (history == nullptr) {
      history = new ObjectHistory(obj_meta->object_id,
                                  BoundingBox(Point(obj_bbox->left, obj_bbox->top),
                                              Point(obj_bbox->left + obj_bbox->width,
                                                    obj_bbox->top + obj_bbox->height)));
      source_info->object_histories[obj_meta->object_id] = history;
    } else {
      auto new_bbox = BoundingBox(Point(obj_bbox->left, obj_bbox->top),
                                  Point(obj_bbox->left + obj_bbox->width,
                                        obj_bbox->top + obj_bbox->height));
      history->update_bounding_box(new_bbox);
    }
    history->last_update_frame = frame_meta->frame_num;
  }

  remove_stale_object_history(frame_meta->frame_num);

  update_line_crossing_analysis(frame_meta->frame_num);
}

void Analytic::remove_stale_object_history(gint current_frame) {
  for (auto [source_id, source_info] : source_analytic_infos) {
    for (auto history_it = source_info->object_histories.begin(); history_it != source_info->object_histories.end();) {
      auto& [object_id, object_history] = *history_it;
      if (current_frame - object_history->last_update_frame > STALE_OBJECT_THRESHOLD) {
        delete object_history;
        history_it = source_info->object_histories.erase(history_it);
      } else {
        history_it++;
      }
    }
  }
}

void Analytic::update_line_crossing_analysis(gint current_frame) {
  for (auto& [source_id, line_crossings] : line_crossing_infos) {
    for (auto& analytic_info : line_crossings) {
      for (auto& [object_id, object_history] : source_analytic_infos[source_id]->object_histories) {
        if (object_history->last_update_frame != current_frame) {
          continue;
        }
        LineCrossDirection direction = analytic_info.line.does_object_cross_line(*object_history);
        auto find_ret = std::find(analytic_info.count_directions.begin(),
                                  analytic_info.count_directions.end(),
                                  direction);
        if (find_ret == analytic_info.count_directions.end()) {
          continue;
        }

        analytic_info.crossing_direction_counts[direction]++;

        std::vector<cv::Mat> video_frames;
        auto get_frame_ret = frame_buffer->get_frames(source_id, object_history->last_update_frame, 100, video_frames);

        char filename[16];
        std::sprintf(filename, "test-%lu.mp4", object_id);
        cv::VideoWriter video_writer {filename, cv::VideoWriter::fourcc('a', 'v', 'c', '1'), 25, cv::Size(1920, 1080)};

        for (auto& frame : video_frames) {
          cv::Mat bgr_mat;
          cv::cvtColor(frame, bgr_mat, cv::COLOR_YUV2BGR_NV12);
          video_writer.write(bgr_mat);
        }
        video_writer.release();
      }
    }
  }
}
