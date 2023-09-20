#include "cameracapabilities.h"

#include <vector>
#include <algorithm>

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/gstcapsfeatures.h>
#include <gst/video/video.h>

namespace Capability
{
	int width(const GstStructure *cap)
	{
		int value;
		gst_structure_get_int(cap, "width", &value);
		return value;
	}

	int height(const GstStructure *cap)
	{
		int value;
		gst_structure_get_int(cap, "height", &value);
		return value;
	}

	int area(const GstStructure *cap)
	{
		return width(cap) * height(cap);
	}

	double framerate(const GstStructure *cap)
	{
		int numerator = -1;
		int denominator = 1;
		const auto fieldtype = gst_structure_get_field_type(cap, "framerate");
		if (fieldtype == GST_TYPE_FRACTION_RANGE)
		{
			const auto value = gst_structure_get_value(cap, "framerate");
			const auto max_value = gst_value_get_fraction_range_max(value);
			numerator = gst_value_get_fraction_numerator(max_value);
			denominator = gst_value_get_fraction_denominator(max_value);
		}
		else if (fieldtype == GST_TYPE_FRACTION)
		{
			gst_structure_get_fraction(cap, "framerate", &numerator, &denominator);
		}

		return static_cast<double>(numerator) / static_cast<double>(denominator);
	}
};

CapabilitySelection::CapabilitySelection(const GstCaps *caps) : m_caps{caps}
{
	gst_caps_ref(const_cast<GstCaps *>(m_caps));
}

CapabilitySelection::~CapabilitySelection()
{
	gst_caps_unref(const_cast<GstCaps *>(m_caps));
}

double CapabilitySelection::highestRawFrameRate() const
{
	const unsigned int nCaps = gst_caps_get_size(m_caps);
	double framerate = 0.0;
	for (unsigned int n = 0; n < nCaps; ++n)
	{
		const auto capn = gst_caps_get_structure(m_caps, n);
		if (g_str_equal(gst_structure_get_name(capn), "video/x-raw"))
		{
			if (Capability::framerate(capn) > framerate)
			{
				framerate = Capability::framerate(capn);
			}
		}
	}
	return framerate;
}

GstCaps *CapabilitySelection::highestRawArea(double minimumFramerate) const
{
	const unsigned int nCaps = gst_caps_get_size(m_caps);
	std::vector<std::pair<double, unsigned int>> pixelrates;
	unsigned int nHighest = 0;
	int area = 0;
	for (unsigned int n = 0; n < nCaps; ++n)
	{
		const auto capn = gst_caps_get_structure(m_caps, n);
		const auto format = gst_structure_get_string(capn, "format");
		if (!format)
		{
			continue;
		}
		const auto info = gst_video_format_get_info(gst_video_format_from_string(format));
		if (!info || info->format == GST_VIDEO_FORMAT_UNKNOWN)
		{
			continue;
		}
		if (g_str_equal(gst_structure_get_name(capn), "video/x-raw"))
		{
			if (Capability::area(capn) > area && Capability::framerate(capn) >= minimumFramerate)
			{
				area = Capability::area(capn);
				nHighest = n;
			}
		}
	}
	if (area == 0)
	{
		return nullptr;
	}

	auto selected_caps = gst_caps_copy_nth(m_caps, nHighest);
	const auto selected_caps_structure = gst_caps_get_structure(selected_caps, 0);
	const auto fieldtype = gst_structure_get_field_type(selected_caps_structure, "framerate");
	if (fieldtype == GST_TYPE_FRACTION_RANGE)
	{
		const auto value = gst_structure_get_value(selected_caps_structure, "framerate");
		const auto max_value = gst_value_get_fraction_range_max(value);
		gst_structure_set_value(selected_caps_structure, "framerate", max_value);
	}

	return selected_caps;
}

bool CapabilitySelection::isJpeg(const GstCaps *caps)
{
	auto cap = gst_caps_get_structure(caps, 0);
	auto name = gst_structure_get_name(cap);
	return g_str_equal(name, "image/jpeg");
}
