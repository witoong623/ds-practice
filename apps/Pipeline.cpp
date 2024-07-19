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

constexpr int MAX_FRAME_BUFFER_SIZE = 300;

Pipeline::Pipeline(GMainLoop *loop, gchar *config_filepath): loop(loop),
  _frame_buffer(MAX_FRAME_BUFFER_SIZE), _analytic(&_frame_buffer) {
  pipeline = gst_pipeline_new ("ds-practice-pipeline");
  streammux = gst_element_factory_make ("nvstreammux", "stream-muxer");

  if (!pipeline || !streammux) {
    auto err_msg = "Pipeline or streammux could not be created. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  sources = create_sources(config_filepath);

  pgie = gst_element_factory_make ("nvinfer", "primary-nvinference-engine");
  tracker = gst_element_factory_make ("nvtracker", "tracker");
  nvdslogger = gst_element_factory_make ("nvdslogger", "nvdslogger");
  nvvidconv = gst_element_factory_make ("nvvideoconvert", "nvvideo-converter");
  nv12_filter = gst_element_factory_make ("capsfilter", "nv12-filter");
  branch_tee = gst_element_factory_make ("tee", "buf-tee");

  output_bin = build_output_bin(config_filepath);
  buffering_bin = build_buffering_bin(config_filepath);

  if (!pgie || !nvdslogger || !tiler || !nvvidconv || !nvosd || !sink || !nv12_filter || !branch_tee || !buffering_bin) {
    auto err_msg = "One element could not be created. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  THROW_ON_PARSER_ERROR(nvds_parse_streammux(streammux, config_filepath,"streammux"));
  THROW_ON_PARSER_ERROR(nvds_parse_gie(pgie, config_filepath, "primary-gie"));
  THROW_ON_PARSER_ERROR(nvds_parse_tracker(tracker, config_filepath, "tracker"));

  g_object_set(G_OBJECT(nvvidconv), "nvbuf-memory-type", 3, nullptr);
  GstCaps *nv12_caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=NV12");
  g_object_set(G_OBJECT(nv12_filter), "caps", nv12_caps, nullptr);
  gst_caps_unref(nv12_caps);

  // register_probs();

  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, pipeline_bus_watch, loop);
  gst_object_unref (bus);

  for (auto source : sources) {
    gst_bin_add (GST_BIN (pipeline), source);
  }

  gst_bin_add_many (GST_BIN (pipeline), streammux, pgie, tracker, nvdslogger,
    nvvidconv, nv12_filter, branch_tee, output_bin, buffering_bin, NULL);

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

  // link until branch_tee
  if (!gst_element_link_many(streammux, pgie, tracker, nvdslogger, nvvidconv, nv12_filter, branch_tee, NULL)) {
    auto err_msg = "Elements could not be linked until nv12 filter. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }

  GstPad *tee_buf_src_pad = gst_element_request_pad_simple(branch_tee, "src_%u");
  GstPad *buf_tee_sink_pad = gst_element_get_static_pad(buffering_bin, "sink");
  if (gst_pad_link(tee_buf_src_pad, buf_tee_sink_pad) != GST_PAD_LINK_OK) {
    auto err_msg = "Failed to link tee to buffering bin. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
  gst_object_unref(buf_tee_sink_pad);

  GstPad *tee_output_src_pad = gst_element_request_pad_simple(branch_tee, "src_%u");
  GstPad *output_tee_sink_pad = gst_element_get_static_pad(output_bin, "sink");
  if (gst_pad_link(tee_output_src_pad, output_tee_sink_pad) != GST_PAD_LINK_OK) {
    auto err_msg = "Failed to link tee to output bin. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
  gst_object_unref(output_tee_sink_pad);
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

GstElement *Pipeline::build_buffering_bin(gchar *config_filepath) {
  GstElement *bin = gst_bin_new("buffering-bin");
  buf_queue = gst_element_factory_make ("queue", "buf-queue");
  buf_fakesink = gst_element_factory_make ("fakesink", "buf-fake-renderer");

  if (!bin || !buf_queue || !buf_fakesink) {
    g_printerr("Buffering elements could not be created.\n");
    return nullptr;
  }

  gst_bin_add_many(GST_BIN(bin), buf_queue, buf_fakesink, NULL);

  if (!gst_element_link(buf_queue, buf_fakesink)) {
    g_printerr("Elements in buffering bin could not be linked.\n");
    return nullptr;
  }

  GstPad *pad = gst_element_get_static_pad(buf_queue, "sink");
  if (!gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad))) {
    g_printerr("Failed to add ghost pad in buffering bin.\n");
    return nullptr;
  }
  
  gst_object_unref(pad);

  return bin;
}

GstElement *Pipeline::build_output_bin(gchar *config_filepath) {
  GstElement *bin = gst_bin_new("output-bin");

  sink_queue = gst_element_factory_make ("queue", "sink-queue");
  nvosd = gst_element_factory_make ("nvdsosd", "nv-onscreendisplay");
  tiler = gst_element_factory_make ("nvmultistreamtiler", "nvtiler");
  // sink = gst_element_factory_make ("nveglglessink", "nvvideo-renderer");
  sink = gst_element_factory_make ("fakesink", "fake-renderer");

  if (!bin || !sink_queue || !nvosd || !tiler || !sink) {
    g_printerr("Output elements could not be created.\n");
    return nullptr;
  }

  // TODO: override batch-size of pgie if number of sources isn't equal batch-size
  THROW_ON_PARSER_ERROR(nvds_parse_osd(nvosd, config_filepath,"osd"));
  // TODO: calculate tiler column
  THROW_ON_PARSER_ERROR(nvds_parse_tiler(tiler, config_filepath, "tiler"));
  // THROW_ON_PARSER_ERROR(nvds_parse_egl_sink(sink, config_filepath, "sink"));

  gst_bin_add_many(GST_BIN(bin), sink_queue, nvosd, tiler, sink, NULL);

  if (!gst_element_link_many(sink_queue, nvosd, tiler, sink, NULL)) {
    g_printerr("Elements in output bin could not be linked.\n");
    return nullptr;
  }

  GstPad *pad = gst_element_get_static_pad(sink_queue, "sink");
  gst_element_add_pad(bin, gst_ghost_pad_new("sink", pad));
  gst_object_unref(pad);

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

  GstPad *vidconv_src_pad = gst_element_get_static_pad(nvvidconv, "src");
  if (!vidconv_src_pad) {
    g_print ("Unable to get vidconv's src pad\n");
  } else {
    gst_pad_add_probe (vidconv_src_pad, GST_PAD_PROBE_TYPE_BUFFER,
      frame_buffer_callback_prob, this, nullptr);
  }

  gst_object_unref(vidconv_src_pad);
}
