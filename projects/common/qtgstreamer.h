// Copyright 2017 S2E Software, Systems and Control
//
// Licensed under the Apache License, Version 2.0 the "License";
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef QTGSTREAMER_H
#define QTGSTREAMER_H

#include <gst/gst.h>
#include <gst/gstinfo.h>

/**
 *  Can be used to transfer the GStreamer debug output to
 *  qDebug functions.
 *
 *  This class is a singelton which corresponds to the exiting
 *  of one, and only one GStreamer instance.
 */
class QtGStreamer
{

public:

	/**
	 * Initialize GStreamer while checking errors.
	 * An information is printed with the found GStreamer version.
	 * Returns the singleton instance
	 */
	static QtGStreamer* init(int* argc, char ***argv, int debugLevel);

	/**
	 * Displays msg with QDebug functions
	 */
	static GstBusSyncReply busCallBack(GstBus* bus, GstMessage* msg, gpointer data);

private:

	/**
	 * GStreamer log funtion as needed by gst_debug_add_log_function().
	 * See GstInfo doc for more information.
	 *
	 * This function determines the snding objects name and invokes the
	 * printMessage() slot with the message.
	 */
	static void gstLogFunction(GstDebugCategory *category,
								GstDebugLevel level,
								const gchar *file,
								const gchar *function,
								gint line,
								GObject *object,
								GstDebugMessage *message,
								gpointer user_data) G_GNUC_NO_INSTRUMENT;

	// Make contructors private such that they cannot be used
	QtGStreamer(){}
	QtGStreamer(QtGStreamer const&) = delete;
	QtGStreamer& operator=(QtGStreamer const&) = delete;

	/**
	 * This one and only instance.
	 */
	static QtGStreamer* m_instance;
};

#endif // QTGSTREAMER_H
