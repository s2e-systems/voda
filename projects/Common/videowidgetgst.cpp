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

#include "videowidgetgst.h"

#include <QDebug>
#include <QPainter>

VideoWidgetGst::VideoWidgetGst() :
	m_appSink(nullptr),
	m_immediateUpdate(true)
{

}

void VideoWidgetGst::installAppSink(GstAppSink* appSink, bool enableOnSampleArrived)
{
	if (appSink == nullptr)
	{
		qWarning() << "Appsink not valid";
		return;
	}

	m_appSink = appSink;

	if (enableOnSampleArrived == true)
	{
		g_object_set(appSink,
					 "emit-signals", TRUE,
					NULL);

		g_signal_connect(appSink,
					"new-sample",
					G_CALLBACK(VideoWidgetGst::onSampleArrived), this);
	}
}

void VideoWidgetGst::paintGL()
{
	//Bind the texture that is used by the VideoWidget base
	// to allow transfering data into that texture
	bindTexture();
	pullFromAppSinkAndTransferToTexture(m_appSink);
	// Call the original painting, which does paint the texture
	// that was just filled
	VideoWidgetGLES2::paintGL();

	if (m_immediateUpdate == true)
	{
		update();
	}
}

void VideoWidgetGst::pullFromAppSinkAndTransferToTexture(GstAppSink* appSink)
{
	GstSample* sample = NULL;
	GstBuffer* sampleBuffer = NULL;
	GstMapInfo mapInfo;
	GstCaps* caps = NULL;
	GstStructure* capsStruct = NULL;
	bool ret;
	int width = 0;
	int height = 0;

	GLint const internalFormat = GL_RGBA;
	GLenum externalFormat;
	GLenum const externalType = GL_UNSIGNED_BYTE;

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
	if(sample == NULL)
	{
		return;
	}

	caps = gst_sample_get_caps(sample);
	capsStruct = gst_caps_get_structure(caps, 0 /*index*/);
	if (capsStruct == NULL)
	{
		qWarning("Failed getting structure from caps.");
		return;
	}

	ret = gst_structure_get_int(capsStruct, "width", &width);
	if (ret == false)
	{
		qWarning("Failed getting width from structure.");
		return;
	}
	ret = gst_structure_get_int(capsStruct, "height", &height);
	if (ret == false)
	{
		qWarning("Failed getting height from structure.");
		return;
	}

	setAspectRatio(QSize(width, height));

	//TODO: Use enums for type comparisson
	gchar const* formatg = gst_structure_get_string(capsStruct, "format");
	QString const format(formatg);

	if (format == "RGB")
	{
		externalFormat = GL_RGB;
	}
	else if (format == "RGBA")
	{
		externalFormat = GL_RGBA;
	}
	else
	{
		qWarning() << "Format" << format << "not supported.";
		return;
	}

	sampleBuffer = gst_sample_get_buffer(sample);
	if(sampleBuffer != NULL)
	{
		gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

		auto pixels = static_cast<GLvoid const* >(mapInfo.data);

		glTexImage2D(GL_TEXTURE_2D,
					 0 /*level*/, internalFormat, width, height,
					 0 /*border, "this value must be zero" [OpenGL doc] */,
					 externalFormat, externalType, pixels);

		gst_buffer_unmap(sampleBuffer, &mapInfo);

	}
	gst_sample_unref(sample);
}


GstFlowReturn VideoWidgetGst::onSampleArrived(GstAppSink* appSink, gpointer userData)
{
	Q_UNUSED(appSink)
	// Connot use 'this' directly since it must be a static function
	auto self = static_cast<VideoWidgetGst*>(userData);

	// Use invoke method and do not call it directly since the
	// update function must be called in the GUI thread
	QMetaObject::invokeMethod(self, "update", Qt::QueuedConnection);

	return GST_FLOW_OK;
}

void VideoWidgetGst::setImmediateUpdate(bool immediateUpdate)
{
	m_immediateUpdate = immediateUpdate;
}

