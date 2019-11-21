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


#include <QObject>
#include <QSize>
#include <QString>

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

/**
 * This class mainly provides convenience functions to create
 * a GStreamer pipeline and specific bins with some compound
 * functionality for video transmission.
 * A GStreamer pipeline has a bus attached to it over which messages
 * are send. This messages are catched by the function #Pipeline::busCallBack().
 * This function is installed during the creation of the pipeline.
 *
 * The convenience model of the pipeline here is based on a fork with
 * one source and one or two sinks. Not all of the sinks or sources must
 * be used.
 * If only one sink branch was set the pipeline:
 *
 *  Source I -> Source II  ->  Sink Main I -> Sink Main II
 *  Source I -> Source II  -> Sink Secondary
 *
 * If two sinks are present, queues are auomatically added during the
 * linking proces:
 *
 *  Source I -> Source II -> tee -> Queue -> Sink Main I -> Sink Main II
 *                               |
 *                               -> Queue -> Sink Secondary
 *
 * Function createPipelineFromDescription() or createPipeline() must
 * be called exactly once.
 *
 * TODO: Originally this class was intentianally not inherited by QObject since
 * it was ment to be inherited by multiple QObjects. This may be obsolete
 * and it may be good to used QObject for e.g. add parent relations ships for
 * a controlled destruction.
 */
class Pipeline
{
	// For Qt enum functions
	Q_GADGET

public:

	enum PackagingMode
	{
		PACKAGINGMODE_UNDEFINED,
		PACKAGINGMODE_BYTESTREAM_NAL,
		PACKAGINGMODE_BYTESTREAM_AU,
		PACKAGINGMODE_AVC,
		PACKAGINGMODE_AVC3
	};
	Q_ENUM(PackagingMode)

	/**
	 * Does nothing
	 */
	Pipeline();

	/**
	 * Creates a GStreamer pipeline with gst_pipeline_new() and
	 * installs a bus observer with Pipeline::installBusCallbackFunction().
	 */
	virtual void createPipeline(const QString& name = QString("Pipeline"));

	/**
	 * Sets the state of the pipeline to playing. Pipeline must be linked before.
	 */
	void startPipeline();

	/**
	 * Links the bins (srcBinI, srcBinII, sinkBinMainI, sinkBinMainII, sinkBinSecondary)
	 * depending if they were set. The bins are added to the pipeline before the linking.
	 * Linking of two bins is done by Pipeline::linkBins()
	 */
	void linkPipeline();

	/**
	 * Set the pipeline to the NULL state
	 */
	void stopPipeline();

	/**
	 * Creates an eos event and send it to the pipeline.
	 */
	void injectEOS();

	/**
	 * Creates a pipeline with a decription as used in gst_parse_launch(); that
	 * is the same as used by the gst-launch tool
	 * The creations is checked for errors, if any error occurs it is printed.
	 */
	bool createPipelineFromDescription(QString const& description);

	/**
	 * Creates a GstBin from a description as used in gst-launch.
	 * Such a description is e.g.: "videoconvert ! video/x-raw, format=RGB"
	 * The bin must be given a name.
	 * createGhostPads is directly passed to the GStreamer gst_parse_bin_from_description() function.
	 * The creation checks or errors and prints them in case there is any.
	 */
	static GstBin* binFromDescription(QString const& description, QString const& name, bool createGhostPads = false);

	/**
	 * Returns the GStreamer pipeline
	 */
	GstElement* pipeline() const;

	/**
	 * Gets the name of the GStreamer pipeline and returns it as a string
	 */
	QString name() const;

	/**
	 * Searches the pipeline for a bin with the name given in the argument.
	 * The result is casted to an appsink and returned.
	 */
	GstAppSink* appSink(const QString& name = "AppSink");

	/**
	 * Searches the pipeline for a bin with the name given in the argument.
	 * The result is casted to an appsrc and returned.
	 */
	GstAppSrc* appSrc(const QString& name = "AppSrc");



	/////////////
	/// Bin Creators (Convenience functions)

	/**
	 * Creates a glimagesink element and if requested prepends a videovonvert element.
	 */
	static GstBin* createDisplaySink(bool addConverter = false);

	/**
	 * Creates a appsink element and prepends a videovonvert element.
	 * The sink is configured such that only one buffer is buffered, otherwise
	 * new buffers are dropped.
	 * The sink only accepts a specific raw video format.
	 * The name of the element is fixed with "AppSink"
	 */
	static GstBin* createAppSink();

	/**
	 * Creates a appsrc element that accepts rtp h264 packets.
	 */
	static GstBin* createAppSrc();

	/**
	 * Creates an x264enc element
	 * Some options are passed directly as a paramter to the x264enc element
	 */
	static GstBin* createX264encoder();

	/**
	 * Creates a openh264enc element and attaches and h264parse to it.
	 * With the modeAfterParse the output of the openh264enc can be transformed.
	 * Note: openh264enc only has the capabilty to output bytestream-au
	 */
	static GstBin* createOpenEncoder();

	/**
	 * Creates an omxh264enc element and attaches capsfilters and a h264parse element to it as in createEncoder()
	 */
	static GstBin* createOmxEncoder();

	/**
	 * Creates an avdec_h264 element with a h264parse element prepended anda videoconvert element appended.
	 * The packaging mode is used to enforce the h264parse for specific packaging mode.
	 */
	static GstBin* createAvDecoder();

	/**
	 * Creates a openh264dec element with a h264parse element prepended and videoconvert element appended.
	 * The packaging mode is used to enforce the h264parse for a specific packaging mode.
	 */
	static GstBin* createOpenDecoder(PackagingMode mode = PACKAGINGMODE_UNDEFINED);

	/**
	 * Creates a omxh264dec element with a h264parse element prepended anda videoconvert element appended.
	 */
	static GstBin* createOmxDecoder();

	/**
	 * Creates the elements videoscale ! videoconvert and attaches a capsfilter witht the values that
	 * can be set through the parameters
	 */
	static GstBin* createScaleConvert(QSize const& size = QSize(512, 288), const QString& format = "I420");

	/**
	 * Creates a ksvideosrc with a capsfilter attached. This is suitable only for Windows OS.
	 *
	 * Note: the framerate and resolution of the camera is explicitly set, so there is no
	 * automatic determination of the capabilities of the connected camera.
	 */
	static GstBin* createCamSrc(QSize const& size = QSize(640, 360), int framerate = 20, int deviceIndex = 0);

	/**
	 * Creates a v4lsrc element with an attached capsfilter.
	 * See also createCamSrc() and its notes.
	 */
	static GstBin* createV4lSrc(QSize const& size = QSize(640, 360), int framerate = 20);

	/**
	 * Creates a videotestsrc element with an attached capsfilter
	 */
	static GstBin* createTestSrc(QSize const& size = QSize(1280, 720), int framerate = 12);



	/////////////////
	/// Convenience functions that do Gstreamer bin operations.

	/**
	 * Determines last element downstream in the bin and returns it.
	 * In other words: the element closest to the end of the pipeline
	 */
	static GstElement* getLastElementOfBin(GstBin* bin);

	/**
	 * Iterates through the elements in the bin and returns the first one upstream.
	 * In other words: the element closest to the begining of the pipeline
	 */
	static GstElement* getFirstElementOfBin(GstBin* bin);

	/**
	 * If linkElements is true the last and first elements are taken from the
	 * respective bins (with getLastElementOfBin() and getFirstElementOfBin())
	 * and these elements then linked with gst_element_link().
	 * If linkElements is false, the bins are only casted with GST_ELEMENT_CAST()
	 * before linking.
	 */
	static bool linkBins(GstBin* binSrc, GstBin* binSink, bool linkElements = false);

	/**
	 * Does a forked linking while adding a tee element and 2 queue elements.
	 *
	 *  binSrc -> tee -> queue -> binSink0
	 *               |
	 *                -> queue -> binSink1
	 *
	 * This linkElements has the same function as in  linkBins(GstBin* binSrc, GstBin* binSink, bool linkElements = false)
	 */
	static bool linkBins(GstBin* binSrc, GstBin* binSink0, GstBin* binSink1, bool linkElements = false);

	/**
	 * Adds up to 9 elements to tje bin with gst_bin_add(). If bin is a nullptr
	 * a new bin is created with the name "Bin".
	 * If all elements could be successfully added true is returned,
	 * otherwise false.
	 */
	static bool binAdd(GstBin* bin, GstElement* elem1, GstElement* elem2 = 0, GstElement* elem3 = 0, GstElement* elem4 = 0, GstElement* elem5 = 0, GstElement* elem6 = 0, GstElement* elem7 = 0, GstElement* elem8 = 0, GstElement* elem9 = 0);

	/**
	 * Set the stream-format and alignment fields of the caps to the packagingMode.
	 * In case the mode is set to PACKAGINGMODE_UNDEFINED, the
	 * caps are not altered.
	 */
	static void setCapsPackagingMode(GstCaps* caps, PackagingMode packagingMode);

	/**
	 * Takes the caps filter element with name capsFilterName from the bin and
	 * asigns the packagingMode to it. This is done by using setCapsPackagingMode().
	 */
	static void createAndSetCapsPackagingMode(GstBin* bin, QString const& capsFilterName, PackagingMode packagingMode);

	////////////////
	/// Setter and Getter

	GstBin* getSrcBinI() const;
	void setSrcBinI(GstBin* srcBinI);

	GstBin* getSrcBinII() const;
	void setSrcBinII(GstBin* srcBinII);

	GstBin* getSinkBinMainI() const;
	void setSinkBinMainI(GstBin* sinkBinMainI);

	GstBin* getSinkBinMainII() const;
	void setSinkBinMainII(GstBin* sinkBinMainII);

	GstBin* getSinkBinSecondary() const;
	void setSinkBinSecondary(GstBin* sinkBinSecondary);

	/**
	 * TODO: This function uses QDebug objects to output the messages. Since
	 * the callBack may be invoked by a different thread then the main Qt trhead,
	 * this function may crash!!! FIXME: Include a Qt::queuedConnection similar
	 * to the one used by QtGStreamer.
	 */
	static GstBusSyncReply busCallBack(GstBus* bus, GstMessage* msg, gpointer data);

protected:
	/**
	 * Gets the bus ang flushes it with gst_bus_set_flushing(TRUE). Does not
	 * reset it to not flushing. See GstBus doc for information.
	 */
	virtual void flushBus();

	/**
	 * Installs the busCallBack() function on the bus.
	 */
	virtual void installBusCallbackFunction();

private:
	GstElement* m_pipeline;
	GstBin* m_srcBinI;
	GstBin* m_srcBinII;
	GstBin* m_sinkBinMainI;
	GstBin* m_sinkBinMainII;
	GstBin* m_sinkBinSecondary;
};


#endif // PIPELINE_H
