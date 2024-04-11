#include <gst/gst.h>
#include <glib.h>
#include "gst-nvmessage.h"


gboolean pipeline_bus_watch (GstBus * bus, GstMessage * msg, gpointer data);

void src_newpad_cb (GstElement * decodebin, GstPad * decoder_src_pad, gpointer data);
