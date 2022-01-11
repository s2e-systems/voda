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

#ifndef VideoWidgetPainterGst_H
#define VideoWidgetPainterGst_H

#include <gst/app/gstappsink.h>
#include <QWidget>


class VideoWidgetPainterGst : public QWidget
{
public:

	/**
	 * Checks if appSink is an intance of GstAppSink and configures it to be suitable for display.
	 */
	VideoWidgetPainterGst(GstAppSink* appSink);

private:

	/**
	 * Takes the "last-sample" from the appSink and renders it.
	 */
	void paintEvent(QPaintEvent* /*event*/) override;

	/**
	 * Invokes an update().
	 */
	static GstFlowReturn onSampleArrived(GstAppSink* appSink, gpointer userData);

	GstAppSink* m_appSink;
};

#endif
