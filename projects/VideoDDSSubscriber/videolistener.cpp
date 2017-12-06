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

#include "videolistener.h"

#include <QDebug>
#include <vector>

VideoListener::VideoListener() :
	m_appSrc(nullptr)
{
}

void VideoListener::installAppSrc(GstAppSrc* appSrc)
{
	m_appSrc = appSrc;
}

VideoListener::~VideoListener()
{
	qDebug() << "Deleted VideoListener";
}

void VideoListener::on_requested_deadline_missed(
	dds::sub::DataReader<S2E::Video>&,
	const dds::core::status::RequestedDeadlineMissedStatus&)
{
	qDebug() << "on_requested_deadline_missed";
}

void VideoListener::on_data_available(dds::sub::DataReader<S2E::Video>& reader)
{
	Q_UNUSED(reader)

	GstBuffer* gstBuffer = NULL;
	GstFlowReturn ret;

	// Only process DDS frames if an appsrc is present.
	if (m_appSrc == nullptr)
	{
		// TODO: May make this silent. Since this may be unavoidable if the app src
		// is installed from another thread then this function is called. And hence
		// the appsrc may be installed a short while after.
		qDebug() << "Not processing arrived DDS data. Appsrc not present.";
		return;
	}

	//Check if the GStreamer pipeline is running, by checking the state of the appsrc
	GstState appSrcState = GST_STATE_NULL;
	gst_element_get_state(GST_ELEMENT(m_appSrc), &appSrcState, NULL, 10/*timeout nanosec*/);
	if (appSrcState != GST_STATE_PLAYING && appSrcState != GST_STATE_PAUSED && appSrcState != GST_STATE_READY)
	{
		// Pipeline is not running, silently return;
		qDebug() << "Pipeline not running" << appSrcState;
		return;
	}

	// Read teh samples that came in by DDS and check its validity.
	dds::sub::LoanedSamples<S2E::Video> samples = reader.read();
	if (samples.length() < 1)
	{
		qDebug() << "Samples length";
		return;
	}
	if((*samples.begin()).info().valid() == false)
	{
		qDebug() << "Samples not valid";
		return;
	}

	// It may happen that multiple samples came in in one go, in that
	// case all the samples are pushed into the GStreamer pipeline.
	// (This may be not possible with some DDS quality of service settings
	// but since its straight farward to implement, both behaviours
	// is handles here)
	for (dds::sub::LoanedSamples<S2E::Video>::const_iterator sample = samples.begin();
		 sample < samples.end();
		 ++sample)
	{
		const std::vector<uint8_t> frame = sample->data().frame();
		auto rawDataPtr = reinterpret_cast<const void *>(frame.data());
		auto byteCount = static_cast<const gsize>(sample->data().frame().size());

		// The following lines might be used to enable data handling without copying
		// the arrived data.
		//gstBuffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_LAST /*flags*/,
		//			rawDataPtr /*data*/, byteCount /*maxsize*/, 0 /*offset*/, byteCount /*size*/,
		//		(gpointer)NULL/*user_data*/, &VideoListener::gstBufferDestroyCallBack);

		// Copy the arrived memory from the DDS into a GStreamer buffer
		GstMapInfo mapInfo;
		gstBuffer = gst_buffer_new_allocate(NULL /* no allocator */, byteCount, NULL /* no parameter */);
		gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_WRITE);
		std::memcpy(static_cast<void*>(mapInfo.data), rawDataPtr, byteCount);
		gst_buffer_unmap(gstBuffer, &mapInfo);

		// Push the buffer into the pipeline. Data freeing is now handled by the pipeline
		ret = gst_app_src_push_buffer(m_appSrc, gstBuffer);
		if (ret != GST_FLOW_OK)
		{
			qWarning() << "Something went wrong while injecting fram data into the display pipeline";
			// TODO: If copy-free method is used, here the data should be freed as well
		}

		// Get the buffer fill to check how many video samples have arrived and are not processed.
		// The app src does unfortunetly not have a sample count.
		guint64 bufferFill;
		g_object_get(m_appSrc, "current-level-bytes", &bufferFill, NULL);
		// TODO: May only output this information for debug build or with a switch
		qDebug() << "Received frameNum:" << sample->data().frameNum() << "with size" << byteCount
				 << " appsrc buffer:" << bufferFill;
	}
}

void VideoListener::gstBufferDestroyCallBack(gpointer data)
{
	//TODO: If copy-free method is used, Do whats neccessary to realease memory in DDS
}
