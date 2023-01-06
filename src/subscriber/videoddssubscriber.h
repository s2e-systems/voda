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

#ifndef VIDEODDSSUBSCRIBER_H
#define VIDEODDSSUBSCRIBER_H

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

/**
 */
class VideoDDSsubscriber
{

public:

	/**
	 */
	VideoDDSsubscriber(bool useOmx);
	GstAppSink* displayAppSink() const;
	GstAppSrc* ddsAppSrc() const;
	GstPipeline* pipeline() const;

private:

	GstElement* m_displayAppSink;
	GstElement* m_ddsAppSrc;
	GstElement* m_pipeline;
};

#endif
