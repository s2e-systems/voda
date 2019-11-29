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

Pipeline::Pipeline() :
m_pipeline(nullptr),
m_srcBinI(nullptr),
m_srcBinII(nullptr),
m_sinkBinMainI(nullptr),
m_sinkBinMainII(nullptr),
m_sinkBinSecondary(nullptr)
{

}

void Pipeline::createPipeline(const QString& name)
{
	if (m_pipeline == nullptr)
	{
		m_pipeline = gst_pipeline_new(qPrintable(name));
		installBusCallbackFunction();
	}
	else
	{
		qWarning() << "Pipeline is already created";
	}
}

void Pipeline::startPipeline()
{
	GstStateChangeReturn ret;

	if (m_pipeline == nullptr )
	{
		qWarning() << "Pipeline must be created first";
		return;
	}

	ret = gst_element_set_state(GST_ELEMENT_CAST(m_pipeline), GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to playing failed";
		flushBus();
	}
}

void Pipeline::linkPipeline()
{
	bool ret = false;

	if (m_pipeline == nullptr)
	{
		qWarning() << "Pipeline must be created first";
		return;
	}

	if (m_srcBinI == nullptr && m_srcBinII == nullptr)
	{
		qWarning() << "Not linking pipeline: At least srcBinI or srcBinII must be installed";
		return;
	}
	if (m_sinkBinMainI == nullptr && m_sinkBinMainII == nullptr && m_sinkBinSecondary == nullptr)
	{
		qWarning() << "Not linking pipeline: At least sinkBinMainI, sinkBinMainII, or sinkBinSecondary must be installed";
		return;
	}

	ret = binAdd(GST_BIN_CAST(m_pipeline), GST_ELEMENT_CAST(m_srcBinI), GST_ELEMENT_CAST(m_srcBinII),
	GST_ELEMENT_CAST(m_sinkBinMainI), GST_ELEMENT_CAST(m_sinkBinMainII), GST_ELEMENT_CAST(m_sinkBinSecondary));
	if (ret == false)
	{
		qWarning() << "Pipeline: Failed to add the bins to the pipeline";
		return;
	}


	GstBin* srcBin = m_srcBinI;
	// Link srcBinI with srcBinII
	if (m_srcBinII != nullptr)
	{
		ret = linkBins(m_srcBinI, m_srcBinII, true);
		srcBin = m_srcBinII;
	}

	GstBin* sinkBinMain = nullptr;
	// Link sinkBinMainI with sinkBinMainII
	if (m_sinkBinMainI != nullptr && m_sinkBinMainII != nullptr)
	{
		ret = linkBins(m_sinkBinMainI, m_sinkBinMainII, true);
		sinkBinMain = m_sinkBinMainI;
	}
	else
	if (m_sinkBinMainI != nullptr && m_sinkBinMainII == nullptr)
	{
		sinkBinMain = m_sinkBinMainI;
	}
	else
	if (m_sinkBinMainI == nullptr && m_sinkBinMainII != nullptr)
	{
		sinkBinMain = m_sinkBinMainII;
	}



	// link src bin with sink bin(s)
	if (sinkBinMain != nullptr && m_sinkBinSecondary != nullptr)
	{
		// Two sinks available (main and secondary sink)
		ret = linkBins(srcBin, sinkBinMain, m_sinkBinSecondary, true);
	}
	else if (sinkBinMain != nullptr && m_sinkBinSecondary == nullptr)
	{
		// One sink available (main sink)
		ret = linkBins(srcBin, sinkBinMain, true);
	}
	else if (sinkBinMain == nullptr && m_sinkBinSecondary != nullptr)
	{
		// One sink available (secondary sink)
		ret = linkBins(srcBin, m_sinkBinSecondary, true);
	}

	if (ret == false)
	{
		flushBus();
	}
}

void Pipeline::stopPipeline()
{
	GstStateChangeReturn ret;

	if (m_pipeline == nullptr)
	{
		return;
	}

	ret = gst_element_set_state(GST_ELEMENT_CAST(m_pipeline), GST_STATE_NULL);
	if (ret == GST_STATE_CHANGE_FAILURE)
	{
		qWarning() << "Set pipeline to NULL failed";

	}
}

void Pipeline::injectEOS()
{
	gboolean ret;

	if (m_pipeline != nullptr)
	{
		ret = gst_element_send_event(m_pipeline, gst_event_new_eos());
		if (ret == FALSE)
		{
			qWarning() << "Could not send EOS event to GStreamer pipeline";
		}
	}
}

GstElement* Pipeline::pipeline() const
{
	return m_pipeline;
}



GstElement* Pipeline::getLastElementOfBin(GstBin* bin)
{
	GstIterator* itr = gst_bin_iterate_sorted(bin);
	GstElement* binElement = nullptr;
	GValue elem = G_VALUE_INIT;
	GstIteratorResult itrRet = gst_iterator_next(itr, &elem);
	if (itrRet == GST_ITERATOR_OK)
	{
		binElement = GST_ELEMENT(g_value_get_object(&elem));
		g_value_reset(&elem);
	}
	gst_iterator_free(itr);

	return binElement;
}

GstElement* Pipeline::getFirstElementOfBin(GstBin* bin)
{
	GstIterator* itr = gst_bin_iterate_sorted(bin);
	GstElement* binElement = 0;
	GValue elem = G_VALUE_INIT;
	while (gst_iterator_next(itr, &elem) == GST_ITERATOR_OK)
	{
		binElement = GST_ELEMENT(g_value_get_object(&elem));
		g_value_reset(&elem);
	}

	g_value_unset(&elem);
	gst_iterator_free(itr);

	return binElement;
}


bool Pipeline::linkBins(GstBin* binSrc, GstBin* binSink, bool linkElements)
{
	gboolean boolret;
	GstElement* binSrcLast = NULL;
	GstElement* binSinkFirst = NULL;

	if (linkElements == true)
	{
		binSrcLast = getLastElementOfBin(binSrc);
		binSinkFirst = getFirstElementOfBin(binSink);
	}
	else
	{
		binSrcLast = GST_ELEMENT_CAST(binSrc);
		binSinkFirst = GST_ELEMENT_CAST(binSink);
	}
	boolret = gst_element_link(binSrcLast, binSinkFirst);


	if (boolret == false)
	{
		gchar* const nameSrc = gst_element_get_name(binSrc);
		gchar* const nameSink = gst_element_get_name(binSink);
		qWarning("Failed to link %s with %s", nameSrc, nameSink);
		g_free(nameSrc);
		g_free(nameSink);
	}

	return boolret;
}


bool Pipeline::linkBins(GstBin* binSrc, GstBin* binSink0, GstBin* binSink1, bool linkElements)
{
	gboolean boolret;
	GstElement* binSrcLast;
	GstElement* binSink0First;
	GstElement* binSink1First;

	if (linkElements == true)
	{
		binSrcLast = getLastElementOfBin(binSrc);
		binSink0First = getFirstElementOfBin(binSink0);
		binSink1First = getFirstElementOfBin(binSink1);
	}
	else
	{
		binSrcLast = GST_ELEMENT_CAST(binSrc);
		binSink0First = GST_ELEMENT_CAST(binSink0);
		binSink1First = GST_ELEMENT_CAST(binSink1);
	}

	GstElement* tee = gst_element_factory_make("tee", "tee");
	GstElement* queue0 = gst_element_factory_make("queue", "queue0");
	GstElement* queue1 = gst_element_factory_make("queue", "queue1");

	gst_bin_add(GST_BIN(binSrc), tee);
	gst_bin_add(GST_BIN(binSink0), queue0);
	gst_bin_add(GST_BIN(binSink1), queue1);

	boolret = gst_element_link(binSrcLast, tee);
	boolret &= gst_element_link(queue0, binSink0First);
	boolret &= gst_element_link(queue1, binSink1First);
	boolret &= gst_element_link_pads(tee, "src_0", queue0, "sink");
	boolret &= gst_element_link_pads(tee, "src_1", queue1, "sink");

	return boolret;
}

void Pipeline::setCapsPackagingMode(GstCaps* caps, Pipeline::PackagingMode packagingMode)
{
	// Note: There is explicitly no alignment set for avc's
	switch (packagingMode)
	{
	case PACKAGINGMODE_UNDEFINED:
		break;
	case PACKAGINGMODE_AVC:
		gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "avc",
		NULL);
		break;
	case PACKAGINGMODE_AVC3:
		gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "avc3",
		NULL);
		break;
	case PACKAGINGMODE_BYTESTREAM_AU:
		gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "au",
		NULL);
		break;
	case PACKAGINGMODE_BYTESTREAM_NAL:
		gst_caps_set_simple(caps, "stream-format", G_TYPE_STRING, "byte-stream",
		"alignment", G_TYPE_STRING, "nal",
		NULL);
		break;
	default:
		qWarning("PackagingMode \"%d\" does not exist", packagingMode);
		break;
	}
}

void Pipeline::createAndSetCapsPackagingMode(GstBin* bin, const QString& elementName, Pipeline::PackagingMode packagingMode)
{
	GstElement* capsFilter;
	GstCaps* caps;

	capsFilter = gst_bin_get_by_name(bin, qPrintable(elementName));
	if (capsFilter == NULL)
	{
		qWarning() << "Element" << elementName << "not found in bin";
		return;
	}
	caps = gst_caps_new_empty_simple("video/x-h264");
	setCapsPackagingMode(caps, packagingMode);
	g_object_set(capsFilter, "caps", caps, NULL);
}

bool Pipeline::binAdd(GstBin* bin, GstElement* elem1, GstElement* elem2, GstElement* elem3, GstElement* elem4, GstElement* elem5, GstElement* elem6, GstElement* elem7, GstElement* elem8, GstElement* elem9)
{
	if (bin == nullptr)
	{
		bin = GST_BIN_CAST(gst_bin_new("Bin"));
	}

	gboolean ret = TRUE;
	QVector<GstElement*> elements(9);
	elements << elem1 << elem2 << elem3 << elem4 << elem5 << elem6 << elem7 << elem8 << elem9;

	foreach (GstElement* element, elements)
	{
		if (element != 0)
		{
			ret &= gst_bin_add(bin, element);
		}
	}
	return ret;
}


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

void Pipeline::flushBus()
{
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	gst_bus_set_flushing(bus, TRUE);
	gst_object_unref(bus);
}

void Pipeline::installBusCallbackFunction()
{
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
	gst_bus_set_sync_handler(bus, busCallBack /*function*/, static_cast<gpointer>(this), NULL /*notify function*/);
	gst_object_unref(bus);
}

GstBin* Pipeline::getSinkBinSecondary() const
{
	return m_sinkBinSecondary;
}

void Pipeline::setSinkBinSecondary(GstBin* sinkBinSecondary)
{
	m_sinkBinSecondary = sinkBinSecondary;
}

GstBin* Pipeline::getSinkBinMainII() const
{
	return m_sinkBinMainII;
}

void Pipeline::setSinkBinMainII(GstBin* sinkBinMainII)
{
	m_sinkBinMainII = sinkBinMainII;
}

GstBin* Pipeline::getSinkBinMainI() const
{
	return m_sinkBinMainI;
}

void Pipeline::setSinkBinMainI(GstBin* sinkBinMainI)
{
	m_sinkBinMainI = sinkBinMainI;
}

GstBin* Pipeline::getSrcBinII() const
{
	return m_srcBinII;
}

void Pipeline::setSrcBinII(GstBin* srcBinII)
{
	m_srcBinII = srcBinII;
}

GstBin* Pipeline::getSrcBinI() const
{
	return m_srcBinI;
}

void Pipeline::setSrcBinI(GstBin* srcBinI)
{
	m_srcBinI = srcBinI;
}

