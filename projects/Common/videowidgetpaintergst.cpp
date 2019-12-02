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

#include "videowidgetpaintergst.h"

#include <QDebug>
#include <QPainter>

VideoWidgetPainterGst::VideoWidgetPainterGst(GstAppSink* appSink) :
	m_appSink(appSink)
{
	if (appSink == nullptr)
	{
		qWarning() << "Appsink not valid";
		return;
	}

	// enable OnSampleArrived
	g_object_set(appSink,
				 "emit-signals", true,
				nullptr);

	g_signal_connect(appSink,
				"new-sample",
				G_CALLBACK(VideoWidgetPainterGst::onSampleArrived), this);
}

void VideoWidgetPainterGst::paintEvent(QPaintEvent* /*event*/)
{
	 pullFromAppSinkAndPaint(m_appSink);
}

void VideoWidgetPainterGst::pullFromAppSinkAndPaint(GstAppSink* appSink)
{
	GstSample* sample = nullptr;
	GstBuffer* sampleBuffer = nullptr;
	GstMapInfo mapInfo;
	GstCaps* caps = nullptr;
	GstStructure* capsStruct = nullptr;
	bool ret;
	int arrivedWidth = 0;
	int arrivedHeight = 0;
	QImage::Format externalFormat = QImage::Format_RGB888;

	if(appSink == nullptr)
	{
		// Return silently.
		// No warning is printed since this function may be called from a different
		// thread than the appsink installation. To allow that behaviour, no
		// disturbing error messages need to be printed.
		return;
	}

	// Pull a sample from the GStreamer pipeline
	const GstClockTime timeOutNanoSeconds = 10000;
	sample = gst_app_sink_try_pull_sample(appSink, timeOutNanoSeconds);
	if(sample == nullptr)
	{
		return;
	}

	caps = gst_sample_get_caps(sample);
	capsStruct = gst_caps_get_structure(caps, 0 /*index*/);
	if (capsStruct == nullptr)
	{
		qWarning("Failed getting structure from caps.");
		return;
	}

	ret = gst_structure_get_int(capsStruct, "width", &arrivedWidth);
	if (ret == false)
	{
		qWarning("Failed getting width from structure.");
		return;
	}
	ret = gst_structure_get_int(capsStruct, "height", &arrivedHeight);
	if (ret == false)
	{
		qWarning("Failed getting height from structure.");
		return;
	}

	//TODO: Use enums for type comparisson
	gchar const* formatg = gst_structure_get_string(capsStruct, "format");
	QString const format(formatg);

	if (format == "RGB")
	{
		externalFormat = QImage::Format_RGB888;
	}
	else if (format == "RGBA")
	{
		externalFormat = QImage::Format_RGBA8888;
	}
	else
	{
		qWarning() << "Format" << format << "not supported.";
		return;
	}

	sampleBuffer = gst_sample_get_buffer(sample);
	if(sampleBuffer != nullptr)
	{
		gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);


		QPainter p{this};

		auto data{const_cast<uchar const* const>(mapInfo.data)};
		/*const auto bytesPerLine{int(image.step)};*/
		const QImage qimage{data, arrivedWidth, arrivedHeight/*, bytesPerLine*/, externalFormat};

		const QSize imageSize{arrivedWidth, arrivedHeight};
		QRect vpRect{QPoint{0, 0}, imageSize.scaled(width(), height(), Qt::KeepAspectRatio)};
		vpRect.moveCenter(rect().center());

		p.drawImage(vpRect, qimage);

		gst_buffer_unmap(sampleBuffer, &mapInfo);

	}
	gst_sample_unref(sample);

//    update();
//    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
}


GstFlowReturn VideoWidgetPainterGst::onSampleArrived(GstAppSink* appSink, gpointer userData)
{
	Q_UNUSED(appSink)
	// Connot use 'this' directly since it must be a static function
	auto self = static_cast<VideoWidgetPainterGst*>(userData);

	// Use invoke method and do not call it directly since the
	// update function must be called in the GUI thread
	QMetaObject::invokeMethod(self, "update", Qt::QueuedConnection);

	return GST_FLOW_OK;
}

