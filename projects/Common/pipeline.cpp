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

#include "pipeline.h"

#include <QDebug>

GstBusSyncReply Pipeline::busCallBack(GstBus* bus, GstMessage* msg, gpointer data)
{
	Q_UNUSED(bus)
	Q_UNUSED(data)

	gchar* debug = NULL;
	GError* err = NULL;

	GstObject const* msgSrcObj = msg->src;
	GstObject const* parentObj = NULL;
	QString objName;
	QString parentObjName;

	if (msgSrcObj != NULL)
	{
		objName = QString(GST_OBJECT_NAME(msgSrcObj));

		parentObj = gst_object_get_parent(GST_OBJECT_CAST(msgSrcObj));
		if (parentObj != NULL)
		{
			parentObjName = QString(GST_OBJECT_NAME(parentObj));
		}
	}

	QString srcObjName;
	if (parentObjName.isEmpty() == false)
	{
		srcObjName += parentObjName + ":";
	}
	if (objName.isEmpty() == false)
	{
		srcObjName += objName + ":";
	}

	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS:
		qDebug().noquote() << srcObjName << "End-of-stream";
		break;
	case GST_MESSAGE_ERROR:
		gst_message_parse_error(msg, &err, &debug);

		qWarning().noquote() << srcObjName << err->message;
		g_error_free(err);

		if (debug != NULL)
		{
			qWarning("Debug details: %s", debug);
			g_free(debug);
		}
		break;
	case GST_MESSAGE_WARNING:
		gst_message_parse_warning(msg, &err, &debug);

		qWarning().noquote() << srcObjName << err->message;
		g_error_free(err);

		if (debug != NULL)
		{
			qWarning("Debug details: %s", debug);
			g_free(debug);
		}
		break;
	case GST_MESSAGE_STATE_CHANGED:
		GstState oldState;
		GstState newState;
		gst_message_parse_state_changed(msg, &oldState, &newState, NULL);
		qDebug().noquote() << QString("GST-STATE-CHANGED (%1) %2 -> %3")
		.arg(srcObjName)
		.arg(gst_element_state_get_name(oldState))
		.arg(gst_element_state_get_name(newState));
		break;
	default:
		//qDebug() << "Bus message arrived of type" << GST_MESSAGE_TYPE(msg);
		break;
	}

	return GST_BUS_PASS;
}


