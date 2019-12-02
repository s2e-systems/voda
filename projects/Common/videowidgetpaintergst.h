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
	 */
	VideoWidgetPainterGst(GstAppSink* appSink);

private:

	void paintEvent(QPaintEvent* /*event*/) override;
	/**
	 * Pulls a GStreamer buffer from the appsink and transferes it to the
	 * GPU using glTexImage2D(GL_TEXTURE_2D, ...).
	 * The format (which pixel storage type) of the buffer is checked and
	 * accoring that the transfer to GPU is handled. If a format is not
	 * supported, a warning is reported.
	 * Since the data is transfered to the GPU, the buffer is freed after that.
	 *
	 * If no appsink is installed, this function siliently returns.
	 */
	void pullFromAppSinkAndPaint(GstAppSink* appSink);

	/**
	 * Only invokes an update().
	 * Can be used to invoke an update() everytime a sample is received.
	 * Note however that this may not be called enough times to empty a
	 * possible queue in the app src, since some scheduled updates may not be
	 * executed.
	 */
	static GstFlowReturn onSampleArrived(GstAppSink* appSink, gpointer userData);

	GstAppSink* m_appSink;
};

#endif
