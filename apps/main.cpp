/*
 * SPDX-FileCopyrightText: Copyright (c) 2018-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <gst/gst.h>
#include <glib.h>

#include <stdexcept>

#include "Pipeline.h"

int
main (int argc, char *argv[])
{
  gst_init (&argc, &argv);
  GMainLoop *loop = g_main_loop_new (nullptr, FALSE);

  Pipeline *pipeline = nullptr;

  try {
    pipeline = new Pipeline(loop, argv[1]);
  } catch (const std::runtime_error &e) {
    g_main_loop_unref (loop);
    return -1;
  }

  g_print ("Running...\n");
  gst_element_set_state (pipeline->pipeline, GST_STATE_PLAYING);
  g_main_loop_run (loop);

  // clean up
  gst_element_set_state (pipeline->pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline->pipeline));
  g_source_remove (pipeline->bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
