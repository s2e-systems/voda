#include "elements.h"

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

	void add(GstBin* bin, std::vector<GstElement*> elements)
	{
		for (auto element : elements)
		{
			const auto ret = gst_bin_add(bin, element);
			if (ret == false)
			{
				throw std::runtime_error{"Adding element to bin failed."};
			}
		}
	}


	void link(std::vector<GstElement*> elements)
	{
		for (auto elementItr = elements.begin(); elementItr != elements.end(); elementItr++)
		{

			if (elementItr+1 != elements.end())
			{
				auto element = *elementItr;
				auto nextElement = *(elementItr+1);
				const auto ret = gst_element_link(element, nextElement);
				if (!ret)
				{
					const auto name = std::string{gst_object_get_name(GST_OBJECT(element))};
					const auto nextName = std::string{gst_object_get_name(GST_OBJECT(nextElement))};
					throw std::runtime_error{"Linking elements " + name + " and " + nextName + " failed."};
				}
			}
		}
	}

	void installGhost(GstBin* bin, GstElement* element, const std::string& padName)
	{
		auto pad = gst_element_get_static_pad(element, padName.c_str());
		// Create ghost pad with the same name
		auto addPadRet = gst_element_add_pad(GST_ELEMENT_CAST(bin), gst_ghost_pad_new(padName.c_str(), pad));
		gst_object_unref(GST_OBJECT(pad));
		if (addPadRet == false)
		{
			throw std::runtime_error("Adding ghost pad to bin failed");
		}
	}
}


ElementSelection::ElementSelection(const std::vector<std::string>& candidates, const std::string& name)
{
	GstElementFactory* factory = nullptr;
	auto candidate = candidates.begin();
	for (; candidate < candidates.end(); candidate++)
	{
		factory = gst_element_factory_find(candidate->c_str());
		if (factory != nullptr)
		{
			break;
		}
	}
	if (factory == nullptr)
	{
		throw std::runtime_error("None of the candidates available.");
	}
	m_element = gst_element_factory_create(factory, name.c_str());
}

std::string ElementSelection::elementName() const
{
	auto name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(gst_element_get_factory(m_element)));
	return {name};
}

GstElement* ElementSelection::element()
{
	return m_element;
}



struct Bin
{
	Bin(const std::string& name) :
		m_bin{GST_BIN_CAST(gst_bin_new(name.c_str()))}
	{}
	operator GstElement*() {return GST_ELEMENT_CAST(m_bin);}
	operator GstBin*() {return m_bin;}
private:
	GstBin* m_bin;
};

struct CapsFilter
{
	CapsFilter(GstCaps* caps)
	{
		m_element = gst_element_factory_make("capsfilter", nullptr);
		g_object_set(m_element, "caps", caps, nullptr);
	}
	operator GstElement*() {return m_element;}
private:
	GstElement* m_element;
};

struct TestSourceJpeg : Bin
{
	TestSourceJpeg(const std::string& name) :
		Bin{name}
	{
		auto source = gst_element_factory_make("videotestsrc", "videosource");
		auto encoder = gst_element_factory_make("jpegenc", "encoder");
		voda::add(*this, {source, encoder});
		voda::installGhost(*this, encoder, "src");
		const auto ret = gst_element_link_many(source, encoder, nullptr);
		if (ret == false)
		{
			throw std::runtime_error("Linking elements failed");
		}
	}
};


struct Source : Bin
{
	Source(GstElement* source, GstCaps* filter , const std::string& name) :
		Bin{name}
	{
		auto converter = gst_element_factory_make("videoconvert", nullptr);
		voda::add(*this, {source, converter});
		voda::installGhost(*this, converter, "src");
		const auto ret = gst_element_link_filtered(source, converter, filter);
		if (ret == false)
		{
			throw std::runtime_error("Linking elements failed");
		}
	}
};


struct Encoder : Bin
{
	Encoder(const std::string& name) :
		Bin{name},
		m_candidates{"omxh264enc", "x264enc", "openh264enc"}
	{
		const auto bitrate = 128;

		ElementSelection selection{m_candidates, "encoder"};
		std::cout << "Selected encoder:" << selection.elementName() << std::endl;

		if (selection.elementName() == "x264enc")
		{
			g_object_set(selection.element(),
				"bitrate", bitrate, // Bitrate in kbit/sec
				"intra-refresh", true, // Use Periodic Intra Refresh instead of IDR frames
				"byte-stream", false, //Generate byte stream format of NALU
				"vbv-buf-capacity", 2000, //Size of the VBV buffer in milliseconds
				"key-int-max", 10, //Maximal distance between two key-frames (0 for automatic)
				"threads", 1, //Number of threads used by the codec (0 for automatic)
				"sliced-threads", false, //Low latency but lower efficiency threading
				"aud", false, //Use AU (Access Unit) delimiter
			nullptr);
			gst_util_set_object_arg(G_OBJECT(selection.element()), "tune", "zerolatency");

//			auto capsFilter = gst_element_factory_make("capsfilter", nullptr);
//			auto caps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "avc", nullptr);
//			g_object_set(capsFilter, "caps", caps, nullptr);
//			auto parser = gst_element_factory_make("h264parse", nullptr);
//			g_object_set(parser, "config-interval", 1, nullptr);
			voda::add(*this, {selection.element()/*, capsFilter, parser*/});


//			const auto ret = gst_element_link_many(selection.element()/*, capsFilter, parser*/, nullptr);
//			if (ret == false)
//			{
//				throw std::runtime_error("Linking elements failed");
//			}
			voda::installGhost(*this, selection.element(), "src");
			voda::installGhost(*this, selection.element(), "sink");
		}
		else
		{
			throw std::runtime_error("Selected encoder not supported yet");
		}
	}

private:
	const std::vector<std::string> m_candidates;
};


struct Decoder : Bin
{
	Decoder(const std::string& name) :
		Bin{name},
		m_candidates{"omxh264dec", "avdec_h264", "openh264dec"}
	{
		ElementSelection selection{m_candidates, "decoder"};
		std::cout << "Selected decoder:" << selection.elementName() << std::endl;

		if (selection.elementName() == "avdec_h264")
		{
			auto parser = gst_element_factory_make("h264parse", nullptr);
			auto capsFilter = gst_element_factory_make("capsfilter", nullptr);
			auto caps = gst_caps_new_simple("video/x-h264", "stream-format", G_TYPE_STRING, "avc", nullptr);
			g_object_set(capsFilter, "caps", caps, nullptr);

			gst_bin_add_many(*this, parser, capsFilter, selection.element(), nullptr);
			gst_element_link_many(parser, capsFilter, selection.element(), nullptr);
			voda::installGhost(*this, parser, "sink");
			voda::installGhost(*this, selection.element(), "src");
		}
		else
		{
			throw std::runtime_error("Selected decoder not supported yet");
		}
	}
private:
	const std::vector<std::string> m_candidates;
};
