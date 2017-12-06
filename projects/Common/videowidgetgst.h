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

#ifndef VIDEOWIDGETGST_H
#define VIDEOWIDGETGST_H

#include <gst/app/gstappsink.h>

#include "videowidgetgles2.h"

/**
 * Extents the VideoWidgetGLES2 by GStreamer functionallity.
 * It is done by inheritance to allow the overwriting of the
 * paintGL() function to pull a sample from an GStreamer
 * appsink element. As such the most recent produced video
 * sample by the GStreamer pipeline can be rendered every time
 * a repaint is neccesarry.
 * The appsink should be configured such that only few (or
 * even one) buffer are allowed in the queue to avoid delays.
 * Also drop buffer should be set to true in the appsink element.
 */
class VideoWidgetGst : public VideoWidgetGLES2
{
public:

	/**
	 * Does nothing.
	 */
	VideoWidgetGst();

	/**
	 * Set the appsink from which pintGL() pulls the video buffers from.
	 * If enableOnSampleArrived is true:
	 * enable emitting of signals of the appsink and connect the onSampleArrived()
	 * callback function.
	 */
	void installAppSink(GstAppSink* appSink, bool enableOnSampleArrived = true);

	/**
	 * If set to true schedules a paint immediatly
	 * after thre painting has finishes.
	 * Default: true
	 */
	void setImmediateUpdate(bool immediateUpdate);

protected:

	/**
	 * 1.) Bind the texture usign bindTexture();
	 * 2.) pullFromAppSinkAndTransferToTexture
	 * Runs the base paint and if immediateUpdate is true,
	 * schedules a repaint immediatly by calling update()
	 */
	virtual void paintGL();

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
	virtual void pullFromAppSinkAndTransferToTexture(GstAppSink* appSink);

private:

	/**
	 * Only invokes an update().
	 * Can be used to invoke an update() everytime a sample is received.
	 * Note however that this may not be called enough times to empty a
	 * possible queue in the app src, since some scheduled updates may not be
	 * executed.
	 */
	static GstFlowReturn onSampleArrived(GstAppSink* appSink, gpointer userData);

	GstAppSink* m_appSink;
	bool m_immediateUpdate;
};

#endif // VIDEOWIDGETGST_H
