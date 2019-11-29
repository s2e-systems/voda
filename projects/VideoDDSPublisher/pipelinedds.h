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
	 * Creates an AppSink GStreamer element that is returned added into a GstBin.
	 * The returned bin can be used to be added in a GStremer pipline.
	 * The AppSink gets the hard-coded name "AppSinkDDS". The appsink element can be
	 * retrieved with that identifier with e.g. the gst_bin_get_by_name() function.
	 *
	 * The AppSink only accepts video/x-h264 data.
	 *  TODO: May be this restriction is not neccessary here. The cap might
	 *   be made fixed at another place. As such arbitrary dat could be sent from
	 *   a GStreamer pipeline
	 *
	 * The AppSink's parameters are configured such that no buffers are buffered
	 * and are dropped if buffers are arriving faster then they are pulled.
	 *
	 * The PipelineDDS::pullSampleAndSendViaDDS() function is connected to
	 * the "new-sample" event of the AppSink. This allows immediate action
	 * to pullout an arrive sample that can be pushed into the DDS writer.
	 */
	GstBin* createAppSinkForDDS();

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
