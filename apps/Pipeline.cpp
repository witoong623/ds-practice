#include "Pipeline.h"

#include <stdexcept>

#include <gst/gst.h>
#include <glib.h>
#include "gst-nvmessage.h"
#include "nvds_yml_parser.h"

#include "Analytic.h"
#include "PipelineCallback.h"


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

  sources = create_sources(config_filepath);

  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");

  tracker = gst_element_factory_make ("nvtracker", "tracker");

  nvdslogger = gst_element_factory_make ("nvdslogger", "nvdslogger");

  tiler = gst_element_factory_make ("nvmultistreamtiler", "nvtiler");

  streammux_tee = gst_element_factory_make ("tee", "streammux-tee");

  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");

  buffer_nvvidconv = gst_element_factory_make ("nvvideoconvert", "buffer-converter");

  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");

  sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");

  buffer_sink = gst_element_factory_make ("fakesink", "buffer-sink");

  if (!streammux_tee || !pgie || !nvdslogger || !tiler || !nvvidconv || !nvosd || !sink) {
    auto err_msg = "One element could not be created. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  THROW_ON_PARSER_ERROR(nvds_parse_streammux(streammux, config_filepath,"streammux"));
  THROW_ON_PARSER_ERROR(nvds_parse_gie(pgie, config_filepath, "primary-gie"));
  THROW_ON_PARSER_ERROR(nvds_parse_tracker(tracker, config_filepath, "tracker"));
  // TODO: override batch-size of pgie if number of sources isn't equal batch-size
  THROW_ON_PARSER_ERROR(nvds_parse_osd(nvosd, config_filepath,"osd"));
  // TODO: calculate tiler column
  THROW_ON_PARSER_ERROR(nvds_parse_tiler(tiler, config_filepath, "tiler"));
  THROW_ON_PARSER_ERROR(nvds_parse_egl_sink(sink, config_filepath, "sink"));

  g_object_set(G_OBJECT(buffer_nvvidconv), "nvbuf-memory-type", 1, nullptr);

  register_probs();

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, pipeline_bus_watch, loop);
  gst_object_unref (bus);

  for (auto source : sources) {
    gst_bin_add (GST_BIN (pipeline), source);
  }

  gst_bin_add_many (GST_BIN (pipeline), streammux, streammux_tee, pgie, tracker, nvdslogger, tiler,
    nvvidconv, nvosd, sink, buffer_nvvidconv, buffer_sink, NULL);

  // link sources to streammux
  for (int i = 0; i < sources.size(); i++) {
    GstElement *source_bin = sources[i];
    GstPad *sinkpad, *srcpad;
    gchar pad_name[16] = { };
    g_snprintf (pad_name, 15, "sink_%u", i);
    sinkpad = gst_element_request_pad_simple (streammux, pad_name);
    if (!sinkpad) {
      auto err_msg = "Streammux request sink pad failed. Exiting.\n";
      g_printerr ("%s", err_msg);
      throw std::runtime_error(err_msg);
    }

    srcpad = gst_element_get_static_pad (source_bin, "src");
    if (!srcpad) {
      auto err_msg = "Failed to get src pad of source bin. Exiting.\n";
      g_printerr ("%s", err_msg);
      throw std::runtime_error(err_msg);
    }

    if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK) {
      auto err_msg = "Failed to link source bin to streammux. Exiting.\n";
      g_printerr ("%s", err_msg);
      throw std::runtime_error(err_msg);
    }

    gst_object_unref (srcpad);
    gst_object_unref (sinkpad);
  }

  // connect streammux, tee and pgie
  // TODO: also with frame buffer related elements
  if (!gst_element_link(streammux, streammux_tee)) {
    auto err_msg = "Streammux and tee could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  GstPad *tee_pgie_src_pad = gst_element_request_pad_simple(streammux_tee, "src_%u");
  GstPad *pgie_sink_pad = gst_element_get_static_pad(pgie, "sink");
  if (!tee_pgie_src_pad || !pgie_sink_pad) {
    auto err_msg = "Request pad from tee or PGIE failed. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
  if (gst_pad_link(tee_pgie_src_pad, pgie_sink_pad) != GST_PAD_LINK_OK) {
    auto err_msg = "Tee and PGIE could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  gst_object_unref(tee_pgie_src_pad);
  gst_object_unref(pgie_sink_pad);

  GstPad *tee_buf_conv_src_pad = gst_element_request_pad_simple(streammux_tee, "src_%u");
  GstPad *buf_conv_sink_pad = gst_element_get_static_pad(buffer_nvvidconv, "sink");
  if (!tee_buf_conv_src_pad || !buf_conv_sink_pad) {
    auto err_msg = "Request pad from tee or buffer converter failed. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  if (gst_pad_link(tee_buf_conv_src_pad, buf_conv_sink_pad) != GST_PAD_LINK_OK) {
    auto err_msg = "Tee and buffer converter could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  gst_object_unref(tee_buf_conv_src_pad);
  gst_object_unref(buf_conv_sink_pad);

  // link the rest of the pipeline
  if (!gst_element_link_many(pgie, tracker, nvdslogger, tiler, nvvidconv, nvosd, sink, NULL)) {
    auto err_msg = "Elements could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  if (!gst_element_link_many(buffer_nvvidconv, buffer_sink, NULL)) {
    auto err_msg = "Buffer elements could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
}

// create a source bin and add it to sources field
std::vector<GstElement *> Pipeline::create_sources(gchar *config_filepath) {
  GList *src_list = NULL ;

  THROW_ON_PARSER_ERROR(nvds_parse_source_list(&src_list, config_filepath, "source-list"));

  // find number of sources
  gint num_sources = 0;
  GList * temp = src_list;
  while(temp) {
    num_sources++;
    temp = temp->next;
  }
  g_list_free(temp);

  std::vector<GstElement *> source_bins;

  for (int i = 0; i < num_sources; i++) {
    GstElement *source_bin = create_source_bin (i, (char*)(src_list)->data);
    if (!source_bin) {
      auto err_msg = "Failed to create source bin. Exiting.\n";
      g_printerr ("%s", err_msg);
      throw std::runtime_error(err_msg);
    }

    source_bins.push_back(source_bin);

    src_list = src_list->next;
  }

  g_free(src_list);

  return source_bins;
}

GstElement *Pipeline::create_source_bin(guint index, gchar *uri) {
  GstElement *bin = NULL, *uri_decode_bin = NULL;
  gchar bin_name[16] = { };

  g_snprintf (bin_name, 15, "source-bin-%02d", index);
  /* Create a source GstBin to abstract this bin's content from the rest of the
   * pipeline */
  bin = gst_bin_new (bin_name);

  /* Source element for reading from the uri.
   * We will use decodebin and let it figure out the container format of the
   * stream and the codec and plug the appropriate demux and decode plugins. */
  uri_decode_bin = gst_element_factory_make ("uridecodebin", "uri-decode-bin");

  if (!bin || !uri_decode_bin) {
    g_printerr ("One element in source bin could not be created.\n");
    return NULL;
  }

  /* We set the input uri to the source element */
  g_object_set (G_OBJECT (uri_decode_bin), "uri", uri, NULL);

  /* Connect to the "pad-added" signal of the decodebin which generates a
   * callback once a new pad for raw data has beed created by the decodebin */
  g_signal_connect (G_OBJECT (uri_decode_bin), "pad-added",
      G_CALLBACK (src_newpad_cb), bin);
  // g_signal_connect (G_OBJECT (uri_decode_bin), "child-added",
  //     G_CALLBACK (decodebin_child_added), bin);

  gst_bin_add (GST_BIN (bin), uri_decode_bin);

  /* We need to create a ghost pad for the source bin which will act as a proxy
   * for the video decoder src pad. The ghost pad will not have a target right
   * now. Once the decode bin creates the video decoder and generates the
   * cb_newpad callback, we will set the ghost pad target to the video decoder
   * src pad. */
  if (!gst_element_add_pad (bin, gst_ghost_pad_new_no_target ("src",
              GST_PAD_SRC))) {
    g_printerr ("Failed to add ghost pad in source bin\n");
    return NULL;
  }

  return bin;
}

void Pipeline::register_probs() {
  GstPad *tiler_sink_pad = gst_element_get_static_pad(tiler, "sink");
  if (!tiler_sink_pad) {
    g_print ("Unable to get tiler's sink pad\n");
  } else {
    gst_pad_add_probe (tiler_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
      analytics_callback_tiler_prob, this, nullptr);
  }

  gst_object_unref(tiler_sink_pad);

  GstPad *buffer_sink_pad = gst_element_get_static_pad(buffer_sink, "sink");
  if (!buffer_sink_pad) {
    g_print ("Unable to get buffer's sink pad\n");
  } else {
    gst_pad_add_probe (buffer_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
      frame_buffer_callback_prob, this, nullptr);
  }

  gst_object_unref(buffer_sink_pad);
}
