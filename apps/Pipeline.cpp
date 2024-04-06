#include "Pipeline.h"

#include <stdexcept>

#include <gst/gst.h>
#include <glib.h>
#include "nvds_yml_parser.h"


// TODO: use logging system to do log

#define THROW_ON_PARSER_ERROR(parse_expr) \
  if (NVDS_YAML_PARSER_SUCCESS != parse_expr) { \
    auto err_msg = "Error in parsing configuration file.\n"; \
    g_printerr("%s", err_msg); \
    throw std::runtime_error(err_msg); \
  }

Pipeline::Pipeline(GMainLoop *loop, gchar *config_filepath): loop(loop) {
  pipeline = gst_pipeline_new ("ds-practice-pipeline");
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux) {
    auto err_msg = "One element could not be created. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  gst_bin_add (GST_BIN (pipeline), streammux);

  // TODO: create sources
  
  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  nvdslogger = gst_element_factory_make ("nvdslogger", "nvdslogger");

  tiler = gst_element_factory_make ("nvmultistreamtiler", "nvtiler");

  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  if (!pgie || !nvdslogger || !tiler || !nvvidconv || !nvosd || !sink) {
    auto err_msg = "One element could not be created. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  THROW_ON_PARSER_ERROR(nvds_parse_streammux(streammux, config_filepath,"streammux"));

  THROW_ON_PARSER_ERROR(nvds_parse_gie(pgie, config_filepath, "primary-gie"));

  // TODO: override batch-size of pgie if number of sources isn't equal batch-size

  THROW_ON_PARSER_ERROR(nvds_parse_osd(nvosd, config_filepath,"osd"));

  // TODO: calculate tiler column

  THROW_ON_PARSER_ERROR(nvds_parse_egl_sink(sink, config_filepath, "sink"));

  // TODO: handle pipline bus

  gst_bin_add_many (GST_BIN (pipeline), pgie, nvdslogger, tiler,
    nvvidconv, nvosd, sink, NULL);

  if (!gst_element_link_many (streammux, pgie, nvdslogger, tiler,
        nvvidconv, nvosd, sink, NULL)) {
    auto err_msg = "Elements could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
}
