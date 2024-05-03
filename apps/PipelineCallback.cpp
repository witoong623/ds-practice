#include "PipelineCallback.h"

#include <glib.h>
#include <gst/gst.h>
#include "gst-nvmessage.h"
#include "gstnvdsmeta.h"

#include "Pipeline.h"


#define GST_CAPS_FEATURES_NVMM "memory:NVMM"

gboolean pipeline_bus_watch (GstBus * bus, GstMessage * msg, gpointer data) {
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

void src_newpad_cb (GstElement * decodebin, GstPad * decoder_src_pad, gpointer data) {
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

GstPadProbeReturn analytics_callback_osd_prob (GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  Pipeline *pipeline = (Pipeline *) user_data;
  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

  pipeline->analytic().update_analytic_state(batch_meta);

  return GST_PAD_PROBE_OK;
}

GstPadProbeReturn drawing_callback_tiler_prob (GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
  // drawing must be done before tiler finish otherwise the coordinate is messup due to tiler resizing
  Pipeline *pipeline = (Pipeline *) user_data;
  GstBuffer *buf = (GstBuffer *) info->data;
  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta (buf);

  pipeline->analytic().draw_on_frame (batch_meta);

  return GST_PAD_PROBE_OK;
}
