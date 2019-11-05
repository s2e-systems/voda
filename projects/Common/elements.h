#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <string>
#include <vector>

#include <iostream>

#include "gst/gst.h"


namespace voda
{
	GstElement* getLastElementOfBin(GstBin* bin)
	{
		auto itr = gst_bin_iterate_sorted(bin);
		GstElement* binElement = nullptr;
		GValue elem = G_VALUE_INIT;
		auto itrRet = gst_iterator_next(itr, &elem);
		gst_iterator_free(itr);
		if (itrRet == GST_ITERATOR_OK)
		{
			binElement = GST_ELEMENT(g_value_get_object(&elem));
			g_value_reset(&elem);
		}
		else
		{
			throw std::range_error("Iterator of bin not OK");
		}

		return binElement;
	}

	GstElement* getFirstElementOfBin(GstBin* bin)
	{
		auto itr = gst_bin_iterate_sorted(bin);
		GstElement* binElement = nullptr;
		GValue elem = G_VALUE_INIT;
		while (gst_iterator_next(itr, &elem) == GST_ITERATOR_OK)
		{
			binElement = GST_ELEMENT(g_value_get_object(&elem));
			g_value_reset(&elem);
		}
		g_value_unset(&elem);
		gst_iterator_free(itr);

		return binElement;
	}
}

struct Source
{
	// GStreamer must be initialized
	Source() :
		m_bin{nullptr},
		m_candidates{"v4l2src", "ksvideosrc", "videotestsrc"}
	{
		m_bin = GST_BIN_CAST(gst_bin_new("sourcebin"));
		GstElementFactory* factory = nullptr;
		auto candidate = m_candidates.begin();
		for (; candidate < m_candidates.end(); candidate++)
		{
			factory = gst_element_factory_find(candidate->c_str());
			if (factory != nullptr)
			{
				break;
			}
		}
		if (factory == nullptr)
		{
			throw std::runtime_error("No videosource available");
		}

		std::cout << "Selected source:" << *candidate << std::endl;

		auto source = gst_element_factory_create(factory, "videosource");
		gst_bin_add(m_bin, source);
	}

	GstBin* bin() {return m_bin;}

private:
	GstBin* m_bin;
	const std::vector<std::string> m_candidates;
};


struct Encoder
{
	// GStreamer must be initialized
	Encoder() :
		m_bin{nullptr},
		m_candidates{"omxh264enc", "x264enc", "openh264enc"}
	{
		const auto bitrate = 128;
		m_bin = GST_BIN_CAST(gst_bin_new("encoderbin"));
		GstElement* encoder = nullptr;
		GstElementFactory* factory = nullptr;
		auto candidate = m_candidates.begin();
		for (; candidate < m_candidates.end(); candidate++)
		{
			factory = gst_element_factory_find(candidate->c_str());
			if (factory != nullptr)
			{
				break;
			}
		}
		if (factory == nullptr)
		{
			throw std::runtime_error("No encoder available");
		}
		std::cout << "Selected encoder:" << *candidate << std::endl;

		if (*candidate == "x264enc")
		{
			encoder = gst_element_factory_create(factory, "encoder");
			g_object_set(encoder,
				"bitrate", bitrate, // Bitrate in kbit/sec
				"intra-refresh", true, // Use Periodic Intra Refresh instead of IDR frames
				"byte-stream", false, //Generate byte stream format of NALU
				"vbv-buf-capacity", 2000, //Size of the VBV buffer in milliseconds
				"key-int-max", 10, //Maximal distance between two key-frames (0 for automatic)
				"threads", 1, //Number of threads used by the codec (0 for automatic)
				"sliced-threads", false, //Low latency but lower efficiency threading
				"aud", false, //Use AU (Access Unit) delimiter
			nullptr);
			gst_util_set_object_arg(G_OBJECT(encoder), "tune", "zerolatency");

			auto capsFilter = gst_element_factory_make("capsfilter", nullptr);
			auto caps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "avc", nullptr);
			g_object_set(capsFilter, "caps", caps, nullptr);
			auto parser = gst_element_factory_make("h264parse", nullptr);
			g_object_set(parser, "config-interval", 1, nullptr);
			gst_bin_add_many(m_bin, encoder, capsFilter, parser, nullptr);
			gst_element_link_many(encoder, capsFilter, parser, nullptr);
		}
		else
		{
			throw std::runtime_error("Selected encoder not supported yet");
		}
	}

	GstBin* bin() {return m_bin;}

private:
	GstBin* m_bin;
	const std::vector<std::string> m_candidates;
};


struct Decoder
{
	// GStreamer must be initialized
	Decoder() :
		m_bin{nullptr},
		m_candidates{"omxh264dec", "avdec_h264", "openh264dec"}
	{
		m_bin = GST_BIN_CAST(gst_bin_new("decoderbin"));
		GstElement* decoder = nullptr;
		GstElementFactory* factory = nullptr;
		auto canditate = m_candidates.begin();
		for (; canditate < m_candidates.end(); canditate++)
		{
			factory = gst_element_factory_find(canditate->c_str());
			if (factory != nullptr)
			{
				break;
			}
		}
		if (factory == nullptr)
		{
			throw std::runtime_error("No decoder available");
		}
		std::cout << "Selected encoder:" << *canditate << std::endl;

		if (*canditate == "avdec_h264")
		{
			auto parser = gst_element_factory_make("h264parse", nullptr);
			auto capsFilter = gst_element_factory_make("capsfilter", nullptr);
			auto caps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "avc", nullptr);
			g_object_set(capsFilter, "caps", caps, nullptr);
			decoder = gst_element_factory_create(factory, "decoder");
			gst_bin_add_many(m_bin, parser, capsFilter, decoder, nullptr);
			gst_element_link_many(parser, capsFilter, decoder, nullptr);
		}
		else
		{
			throw std::runtime_error("Selected decoder not supported yet");
		}
	}

	GstBin* bin() {return m_bin;}

private:
	GstBin* m_bin;
	const std::vector<std::string> m_candidates;
};

#endif
