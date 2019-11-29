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

#ifndef PIPELINEDDS_H
#define PIPELINEDDS_H

#include "pipeline.h"

#include "VideoDDS_DCPS.hpp"

/**
 * Extents the Pipeline by DDS sending capabilities. This enables the handling
 * of compressed data samples asynchronely to be sent via DDS.
 *
 * It provided a function to create an appsink that can be added into
 * a GStreamer pipeline.
 */
class PipelineDDS : public Pipeline
{
public:

	/**
	 * Does nothing.
	 */
	PipelineDDS();

	/**
	 * Set the DDS data writer that is used by pullSampleAndSendViaDDS() to
	 * send the data from the GStreamer buffers into DDS
	 */
	void setDataWriter(const dds::pub::DataWriter<S2E::Video>& dataWriter);

	/**
	 * Takes the data of the appSink and pushes it into the DDS dataWriter.
	 */
	static GstFlowReturn pullSampleAndSendViaDDS(GstAppSink* appSink, gpointer userData);

private:
	dds::pub::DataWriter<S2E::Video> m_dataWriter;
};

#endif // PIPELINEDDS_H
