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

#include <cstring>
#include <stdexcept>

VideoListener::VideoListener(GstAppSrc* const appSrc) :
	m_appSrc(appSrc)
{
	if (appSrc == nullptr || !GST_IS_APP_SRC(appSrc))
	{
		throw std::range_error{"AppSrc not valid"};
	}
}

void VideoListener::on_data_available(dds::sub::DataReader<S2E::Video>& reader)
{
	//Check if the GStreamer pipeline is running, by checking the state of the appsrc
	GstState appSrcState = GST_STATE_NULL;
	gst_element_get_state(GST_ELEMENT(m_appSrc), &appSrcState, nullptr /*pending*/, 10/*timeout nanosec*/);
	if (appSrcState != GST_STATE_PLAYING && appSrcState != GST_STATE_PAUSED && appSrcState != GST_STATE_READY)
	{
		return;
	}

	const auto samples = reader.take();

	// It may happen that multiple samples came in in one go, in that
	// case all the samples are pushed into the GStreamer pipeline.
	for (const auto& sample : samples)
	{
		if(sample.info().valid() == false)
		{
			continue;
		}
		const auto& frame = sample.data().frame();
		const auto rawDataPtr = reinterpret_cast<const void *>(frame.data());
		const auto byteCount = static_cast<const gsize>(sample.data().frame().size());

		// Copy the arrived memory from the DDS into a GStreamer buffer
		GstMapInfo mapInfo;
		auto gstBuffer = gst_buffer_new_allocate(nullptr /* no allocator */, byteCount, nullptr /* no parameter */);
		gst_buffer_map(gstBuffer, &mapInfo, GST_MAP_WRITE);
		std::memcpy(static_cast<void*>(mapInfo.data), rawDataPtr, byteCount);
		gst_buffer_unmap(gstBuffer, &mapInfo);

		// Push the buffer into the pipeline. Data freeing is now handled by the pipeline
		const auto ret = gst_app_src_push_buffer(m_appSrc, gstBuffer);
		if (ret != GST_FLOW_OK)
		{
			return;
		}
	}
}