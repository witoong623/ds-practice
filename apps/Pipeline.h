#pragma once

#include <vector>

#include <gst/gst.h>
#include <glib.h>

#include "Analytic.h"
#include "FrameBuffer.h"

class Pipeline {
  public:
    Pipeline(GMainLoop *loop, gchar *config_filepath);

    ~Pipeline();

    guint bus_watch_id;
    GstElement *pipeline;
    inline Analytic &analytic() { return _analytic; }
    inline FrameBuffer &frame_buffer() { return _frame_buffer; }
  private:
    std::vector<GstElement *> create_sources(gchar *config_filepath);
    GstElement *create_source_bin (guint index, gchar *uri);

    GstElement *build_buffering_bin(gchar *config_filepath);
    GstElement *build_output_bin(gchar *config_filepath);

    // should be called after all elements are configured
    void register_probs();

    GMainLoop *loop;

    GstElement *streammux;
    GstElement *pgie;
    GstElement *tracker;
    GstElement *nvdslogger;
    GstElement *nvvidconv;
    GstElement *nv12_filter;
    GstElement *branch_tee;

    GstElement *sink_queue;
    GstElement *nvosd;
    GstElement *tiler;
    GstElement *sink;
    GstElement *output_bin;

    GstElement *buf_fakesink;
    GstElement *buf_queue;
    GstElement *buffering_bin;

    Analytic _analytic;
    FrameBuffer _frame_buffer;
    std::vector<GstElement *> sources;
};
