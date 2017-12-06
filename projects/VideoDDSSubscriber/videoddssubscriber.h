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

#include <QApplication>


#include "VideoDDS_DCPS.hpp"

//forward declarations
class VideoListener;
class Pipeline;

/**
 * Video subscriber application is inherited from QApplication.
 * This may be chaged in the future to separate the resposibilies.
 * But for this small program it is probably good enough.
 */
class VideoDDSsubscriber : public QApplication
{
	Q_OBJECT

public:

	/**
	 * Set the applciation name (hard coded)
	 */
	VideoDDSsubscriber(int& argc, char** argv);

	/**
	 * @brief initDDS This method initializes the DDS subscriber and data reader
	 * @param topicName Name of the DDS topic on which data is subscribed
	 */
	void initDDS(const QString& topicName);

	/**
	 * @brief init This method initializes the GStreamer pipeline and the widget
	 * for displaying the video
	 */
	void init();

	/**
	 * These methods enable the use of the OMX hardware decoder on the
	 * RaspberryPi. The hardware encoder is needed for undelayed video
	 * processing on the Pi.
	 */
	bool useOmx() const;
	void setUseOmx(bool useOmx);

	/**
	 * These methods enable the test source for the video input
	 * instead of the camera
	 */
	bool useTestSrc() const;
	void setUseTestSrc(bool useTestSrc);

protected:

private:
	VideoListener* m_videoListener;
	Pipeline* m_pipeline;
	dds::sub::DataReader<S2E::Video> m_dr;
	bool m_useOmx;
};

#endif
