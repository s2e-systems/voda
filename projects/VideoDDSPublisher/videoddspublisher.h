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

#include <QApplication>


#include "VideoDDS_DCPS.hpp"

#include "gst/gst.h"

//forward declarations
class QMainWindow;
class PipelineDDS;




struct Display
{
	Display(){}
};


/**
 * Publisher application that owns a specific data writer and
 * a gstreamer pipeline.
 * The data writer can be initialized with initDDS().
 *
 * This application can be convenently initialized by calling init().
 * This function calls the other init..() functons in the necessary order.
 */
class VideoDDSpublisher : public QApplication
{
public:

	/**
	 * Creates the application.
	 * Takes standard arguments from main() arguments to contruct the
	 * inherited QApplication
	 */
	VideoDDSpublisher(int& argc, char *argv[]);

	/**
	 * Initializes the DDS system. Catches errors that may occur.
	 */
	void initDDS(const QString& topicName);

	/**
	 * Initializes the pipeline, creates a widget and starts the pipeline.
	 * Also show the widget!
	 * TODO: May be separate the widget creation and showing from this function.
	 *
	 * TODO: Most of the camera source and encoding parameters are hard coded.
	 * Make them configurable.
	 */
	void initGstreamer();

	/**
	 * Creates a new GStreamer pipeline inits DDS with a hard-coded topic name and
	 * initializes GStreamer.
	 * This is a convenince methods to allow quich initialization.
	 */
	void init();

	/**
	 * These methods enable the test source for the video input
	 * instead of the camera
	 */
	bool useTestSrc() const;
	void setUseTestSrc(bool useTestSrc);

	/**
	 * These methods enable the use of the OMX hardware decoder on the
	 * RaspberryPi. The hardware encoder is needed for undelayed video
	 * processing on the Pi.
	 */
	bool useOmx() const;
	void setUseOmx(bool useOmx);

	/**
	 * These methods define the ownership strength of the topic.
	 * Higher ownership strength means a higher priority for the sender
	 */
	int strength() const;
	void setStrength(int strength);

	/**
	 * These method the define the usage of Video4Linux, necessary
	 * when sampling the camera from the Linux OS
	 */
	bool useV4l() const;
	void setUseV4l(bool useV4l);

protected:

private:

	QMainWindow* m_mainwindow;
	PipelineDDS* m_pipeline;

	/** Data writer: must be initialized since the construtor is private */
	dds::pub::DataWriter<S2E::Video> m_dataWriter;

	bool m_useTestSrc;
	bool m_useOmx;
	bool m_useV4l;
	int m_strength;
};

#endif
