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

#include <QObject>

#include <gst/gst.h>
#include <gst/gstinfo.h>

/**
 *  Can be used to transfer the GStreamer debug output to
 *  qDebug functions.
 *  A compount GStreamer initialization is also provided.
 *
 *  This class is based on QObject which allows the transfer
 *  of the messages from any GStreamer thread to the main
 *  Qt thread, such that qDebug() and such work safely.
 *
 *  This class is a singelton which corresponds to the exiting
 *  of one, and only one GStreamer instance.
 */
class QtGStreamer : public QObject
{
	Q_OBJECT

public:

	/**
	 * Get the one and only instance.
	 */
	static QtGStreamer* instance();

	/**
	 * Initialize GStreamer while checking errors.
	 * An information is printed with the found GStreamer version.
	 */
	static bool init();

	/**
	 * Removes the default debug message handler from GStreamer
	 * and installs QtGstreamer::gstLogFunction()
	 */
	static void installMessageHandler(int level = 1);


public slots:

	/**
	 * Depending on the level, the msg will be passed to qDebug, qWarning,
	 * qInfo or qError.
	 */
	void printMessage(GstDebugLevel level, QString const msg);

protected:

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

	/**
	 * May be used for the install message handler.
	 * At this stage its not used.
	 */
	static void notify(gpointer data){ Q_UNUSED(data) }

private:
	// Make contructors private such that they cannot be used
	QtGStreamer(QObject* parent = 0) : QObject(parent){}
	QtGStreamer(QtGStreamer const&) : QObject(0){}
	QtGStreamer& operator=(QtGStreamer const&){return *this;}

	/**
	 * This one and only instance.
	 */
	static QtGStreamer* m_instance;
};

#endif // QTGSTREAMER_H
