#ifndef CAMERACAPABILITIES_H
#define CAMERACAPABILITIES_H

#include <vector>
#include <algorithm>

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/gstcapsfeatures.h>

namespace Capability
{
	int width(const GstStructure* cap)
	{
		int value;
		gst_structure_get_int(cap, "width", &value);
		return value;
	}

	int height(const GstStructure* cap)
	{
		int value;
		gst_structure_get_int(cap, "height", &value);
		return value;
	}

	int area(const GstStructure* cap)
	{
		return width(cap) * height(cap);
	}

	double framerate(const GstStructure* cap)
	{
		int nominator;
		int denominator;
		gst_structure_get_fraction(cap, "framerate", &nominator, &denominator);
		return static_cast<double>(nominator) / static_cast<double>(denominator);
	}
};

struct CapabilitySelection
{
	CapabilitySelection(const GstCaps* caps) :
		m_caps{caps}
	{
		gst_caps_ref(const_cast<GstCaps*>(m_caps));
	}

	~CapabilitySelection()
	{
		gst_caps_unref(const_cast<GstCaps*>(m_caps));
	}

	double highestRawFrameRate() const
	{
		const unsigned int nCaps = gst_caps_get_size(m_caps);
		double framerate = 0.0;
		for(unsigned int n = 0; n < nCaps; ++n)
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


	GstCaps* highestRawArea(double minimumFramerate = 0) const
	{
		const unsigned int nCaps = gst_caps_get_size(m_caps);
		std::vector<std::pair<double, unsigned int>> pixelrates;
		unsigned int nHighest = 0;
		int area = 0;
		for(unsigned int n = 0; n < nCaps; ++n)
		{
			const auto capn = gst_caps_get_structure(m_caps, n);
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

		return gst_caps_copy_nth(m_caps, nHighest);
	}

	static bool isJpeg(const GstCaps* caps)
	{
		auto cap = gst_caps_get_structure(caps, 0);
		auto name = gst_structure_get_name(cap);
		return g_str_equal(name, "image/jpeg");
	}

private:
	GstCaps const* const  m_caps;
};

#endif
