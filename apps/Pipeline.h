#pragma once

#include <gst/gst.h>
#include <glib.h>


class Pipeline {
  public:
    Pipeline(GMainLoop *loop, gchar *config_filepath);

    ~Pipeline();


  private:
    GMainLoop *loop;

    GstElement *pipeline;
    GstElement *streammux;
    GstElement *sink;
    GstElement *pgie;
    GstElement *nvvidconv;
    GstElement *nvosd;
    GstElement *tiler;
    GstElement *nvdslogger;
};
