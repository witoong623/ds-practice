#pragma once

#include <vector>

#include <gst/gst.h>
#include <glib.h>


class Pipeline {
  public:
    Pipeline(GMainLoop *loop, gchar *config_filepath);

    ~Pipeline();

    guint bus_watch_id;
    GstElement *pipeline;
  private:
    std::vector<GstElement *> create_sources(gchar *config_filepath);
    GstElement *create_source_bin (guint index, gchar *uri);

    GMainLoop *loop;

    GstElement *streammux;
    GstElement *sink;
    GstElement *pgie;
    GstElement *nvvidconv;
    GstElement *nvosd;
    GstElement *tiler;
    GstElement *nvdslogger;

    std::vector<GstElement *> sources;
};
