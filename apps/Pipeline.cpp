#include "Pipeline.h"

#include <stdexcept>

#include <gst/gst.h>
#include <glib.h>
#include "gst-nvmessage.h"
#include "nvds_yml_parser.h"


// TODO: use logging system to do log

#define THROW_ON_PARSER_ERROR(parse_expr) \
  if (NVDS_YAML_PARSER_SUCCESS != parse_expr) { \
    auto err_msg = "Error in parsing configuration file.\n"; \
    g_printerr("%s", err_msg); \
    throw std::runtime_error(err_msg); \
  }

#define GST_CAPS_FEATURES_NVMM "memory:NVMM"

void cb_newpad (GstElement * decodebin, GstPad * decoder_src_pad, gpointer data) {
  GstCaps *caps = gst_pad_get_current_caps (decoder_src_pad);
  if (!caps) {
    caps = gst_pad_query_caps (decoder_src_pad, NULL);
  }
  const GstStructure *str = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (str);
  GstElement *source_bin = (GstElement *) data;
  GstCapsFeatures *features = gst_caps_get_features (caps, 0);

  /* Need to check if the pad created by the decodebin is for video and not
   * audio. */
  if (!strncmp (name, "video", 5)) {
    /* Link the decodebin pad only if decodebin has picked nvidia
     * decoder plugin nvdec_*. We do this by checking if the pad caps contain
     * NVMM memory features. */
    if (gst_caps_features_contains (features, GST_CAPS_FEATURES_NVMM)) {
      /* Get the source bin ghost pad */
      GstPad *bin_ghost_pad = gst_element_get_static_pad (source_bin, "src");
      if (!gst_ghost_pad_set_target (GST_GHOST_PAD (bin_ghost_pad),
              decoder_src_pad)) {
        g_printerr ("Failed to link decoder src pad to source bin ghost pad\n");
      }
      gst_object_unref (bin_ghost_pad);
    } else {
      g_printerr ("Error: Decodebin did not pick nvidia decoder plugin.\n");
    }
  }
}

gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;
    case GST_MESSAGE_WARNING:
    {
      gchar *debug;
      GError *error;
      gst_message_parse_warning (msg, &error, &debug);
      g_printerr ("WARNING from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_free (debug);
      g_printerr ("Warning: %s\n", error->message);
      g_error_free (error);
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_printerr ("ERROR from element %s: %s\n",
          GST_OBJECT_NAME (msg->src), error->message);
      if (debug)
        g_printerr ("Error details: %s\n", debug);
      g_free (debug);
      g_error_free (error);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_ELEMENT:
    {
      if (gst_nvmessage_is_stream_eos (msg)) {
        guint stream_id;
        if (gst_nvmessage_parse_stream_eos (msg, &stream_id)) {
          g_print ("Got EOS from stream %d\n", stream_id);
        }
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
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

  create_sources(config_filepath);
  
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
  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  for (auto source : sources) {
    gst_bin_add (GST_BIN (pipeline), source);
  }

  gst_bin_add_many (GST_BIN (pipeline), pgie, nvdslogger, tiler,
    nvvidconv, nvosd, sink, NULL);

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

  if (!gst_element_link_many (streammux, pgie, nvdslogger, tiler,
        nvvidconv, nvosd, sink, NULL)) {
    auto err_msg = "Elements could not be linked. Exiting.\n";
    g_printerr ("%s", err_msg);
    throw std::runtime_error(err_msg);
  }
}

// create a source bin and add it to sources field
void Pipeline::create_sources(gchar *config_filepath) {
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

  for (int i = 0; i < num_sources; i++) {
    GstElement *source_bin = create_source_bin (i, (char*)(src_list)->data);
    if (!source_bin) {
      auto err_msg = "Failed to create source bin. Exiting.\n";
      g_printerr ("%s", err_msg);
      throw std::runtime_error(err_msg);
    }

    sources.push_back(source_bin);

    src_list = src_list->next;
  }

  g_free(src_list);
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
      G_CALLBACK (cb_newpad), bin);
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
