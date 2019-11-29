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

#include "pipelinedds.h"

#include <QDebug>

PipelineDDS::PipelineDDS() :
	m_dataWriter(dds::core::null)
{
}


GstFlowReturn PipelineDDS::pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData)
{
	Q_UNUSED(appSink)
	bool dataWriterValid = false;
	auto dataWriter = reinterpret_cast<dds::pub::DataWriter<S2E::Video>* >(userData);
	if (dataWriter == nullptr)
	{
		dataWriterValid = false;
	}
	if (dataWriter->is_nil() == true)
	{
		qWarning() << "Data writer in Pipeline not valid";
		dataWriterValid = false;
	}
	else
	{
		dataWriterValid = true;
	}
	//dataWriter->assert_liveliness();

	const int userid = 0;
	// Count the samples that have arrived. Used also to define the
	// DDS msg number later
	static int frameNum = 0;
	frameNum++;

	GstSample* sample = NULL;
	GstBuffer* sampleBuffer = NULL;
	GstMapInfo mapInfo;

	// Pull a sample from the GStreamer pipeline
	sample = gst_app_sink_pull_sample(appSink);
	if(sample != NULL)
	{
		sampleBuffer = gst_sample_get_buffer(sample);
		if(sampleBuffer != NULL && dataWriterValid == true)
		{
			gst_buffer_map(sampleBuffer, &mapInfo, GST_MAP_READ);

			const int byteCount = mapInfo.size;
			auto rawData = static_cast<uint8_t* >(mapInfo.data);
			const std::vector<uint8_t> frame(rawData, rawData + byteCount);
			const S2E::Video sample(userid, frameNum, frame);
			*dataWriter << sample;
			gst_buffer_unmap(sampleBuffer, &mapInfo);

			qDebug() << "Send DDS msg" << frameNum << "with size of" << byteCount << "Bytes";

		}
		// The S2E::Video sample and the std::vector<uint8_t> frame are destroyed here.
		// How ever the DDS system takes care that the necessary data stays alive.
		// As such some data is compied around here.
		// TODO: the copying may be avoided by using other function.
		gst_sample_unref(sample);
	}

	return GST_FLOW_OK;
}

void PipelineDDS::setDataWriter(const dds::pub::DataWriter<S2E::Video>& dataWriter)
{
	if (dataWriter.is_nil() == true)
	{
		qDebug() << "Data writer not valid";
	}
	m_dataWriter = dataWriter;
}
