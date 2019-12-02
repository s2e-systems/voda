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

#ifndef VIDEODDSPUBLISHER_H
#define VIDEODDSPUBLISHER_H

#include "VideoDDS_DCPS.hpp"
#include <gst/gstelement.h>
#include <gst/app/gstappsink.h>


/**
 */
class VideoDDSpublisher
{
public:

	VideoDDSpublisher(dds::pub::DataWriter<S2E::Video>& dataWriter, bool useTestSrc, bool useOmx, bool useFixedCaps);
	GstAppSink* appsink();

private:
	dds::pub::DataWriter<S2E::Video> m_dataWriter;
	GstElement* m_appSink;
};


#endif
