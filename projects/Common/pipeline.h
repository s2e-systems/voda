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

#ifndef PIPELINE_H
#define PIPELINE_H

#include <gst/gst.h>

struct Pipeline
{
	/**
	 * TODO: This function uses QDebug objects to output the messages. Since
	 * the callBack may be invoked by a different thread then the main Qt trhead,
	 * this function may crash!!! FIXME: Include a Qt::queuedConnection similar
	 * to the one used by QtGStreamer.
	 */
	static GstBusSyncReply busCallBack(GstBus* bus, GstMessage* msg, gpointer data);
};

#endif // PIPELINE_H
