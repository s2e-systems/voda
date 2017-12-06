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

#include "qtgstreamer.h"

#include <QDebug>
#include <QCoreApplication>

QtGStreamer* QtGStreamer::m_instance = nullptr;

QtGStreamer* QtGStreamer::instance()
{
	// Only allow one instance
	if (m_instance == nullptr)
	{
		m_instance = new QtGStreamer();
		qRegisterMetaType<GstDebugLevel>("GstDebugLevel");
	}
	return m_instance;
}


void QtGStreamer::gstLogFunction(GstDebugCategory* category,
				   GstDebugLevel level,
				   const gchar* file,
				   const gchar* function,
				   gint line,
				   GObject* object,
				   GstDebugMessage* message,
				   gpointer user_data)
{
	Q_UNUSED(category)
	Q_UNUSED(file)
	Q_UNUSED(function)
	Q_UNUSED(line)
	Q_UNUSED(user_data)

	QString objectName;

	// The QString is contructed here while creating
	// new memory and the original char* message can
	// be destroyed safely.
	// The QString is then passed to the
	// invokeMethod with a queued connection, this will keep a reference to
	// this QString (due to the implicit sharing it uses)
	// and will only be destroyed after the invoked method
	// is finished. Important is only that the
	QString qMsg(gst_debug_message_get(message));

	if (object != NULL)
	{
		// Note that gst_object_get_name() is blocking here.
		// This may be because the object itself is emmitting this
		// function here and the gst_object_get_name() function
		// "grabs and releases object 's LOCK." [GstObject doc]
		// A similar behaviour with gst_object_get_parent(), hence
		// The parents name is not determined here
		// (even if it would be interessting)
		objectName = QString(GST_OBJECT_NAME(object));
	}

	if (objectName.isEmpty() == false)
	{
		qMsg = objectName + ":" + qMsg;
	}

	// Use a queuedconnection to explicitly let the function be executed
	// in the GUI thread, such that the QDebug functions run safely.
	QMetaObject::invokeMethod(QtGStreamer::instance(), "printMessage", Qt::QueuedConnection,
							  Q_ARG(GstDebugLevel, level),
							  Q_ARG(QString, qMsg));
}

void QtGStreamer::printMessage(GstDebugLevel level, const QString msg)
{
	switch (level)
	{
	case GST_LEVEL_NONE:
		qWarning().noquote() << "Gst:" << msg;
		break;
	case GST_LEVEL_ERROR:
	case GST_LEVEL_WARNING:
		qWarning().noquote() << "GstWarn:" << msg;
		break;
	case GST_LEVEL_FIXME:
	case GST_LEVEL_INFO:
	case GST_LEVEL_DEBUG:
		qDebug().noquote() << "GstInfo:" << msg;
		break;
	case GST_LEVEL_LOG:
	case GST_LEVEL_TRACE:
	case GST_LEVEL_MEMDUMP:
		qInfo().noquote() << "GstLog:" << msg;
		break;
	default:
		qInfo().noquote() << "GstDefault:" << msg;
		break;
	}
}

bool QtGStreamer::init()
{
	GError* gerr = NULL;
	gboolean gret;

	if (gst_is_initialized() == FALSE)
	{
		// TODO: pipe application arguments here (if present)
		gret = gst_init_check(NULL /*argc*/, NULL /*argv*/, &gerr);
		if (gret == FALSE)
		{
			qCritical("Could not initialize GStreamer: %s", gerr->message);
			g_error_free(gerr);
			return false;
		}
	}

	guint major;
	guint minor;
	guint micro;
	guint nano;

	gst_version(&major, &minor, &micro, &nano);

	qInfo("GStreamer version %d.%d.%d.%d\n", major, minor, micro, nano);

	return true;
}

void QtGStreamer::installMessageHandler(int level)
{
	if (gst_is_initialized() == TRUE)
	{
		qWarning() << "The message handler must be installed before GStreamer is initialized!";
	}
	else
	{
		// Set the debug level for all classes
		// Can still be overwritten by the GST_DEBUG environment variable
		// This function can be used before gst_init() (as stated in GStreamer doc)
		gst_debug_set_default_threshold(static_cast<GstDebugLevel>(level));

		// Replace the log function
		gst_debug_remove_log_function(&gst_debug_log_default);
		gst_debug_add_log_function(&gstLogFunction, NULL /*user_data*/, NULL /*notify*/);

		// Disables colors of the debug output from gstreamer
		// If the default is used it becomes unreadable on cmd line terminals
		// with white background
		gst_debug_set_color_mode(GST_DEBUG_COLOR_MODE_OFF);
		//Not threadsafe (so this function show be called from the same thread as the
		// init() function was called)
		// "debugging messages are sent to the debugging handlers"
		gst_debug_set_active(TRUE);
	}
}
