#ifndef CAMERACAPABILITIES_H
#define CAMERACAPABILITIES_H

#include <vector>
#include <algorithm>

#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <gst/gstcapsfeatures.h>

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

	GstCaps* highestPixelRate(double minimumFramerate = 0) const
	{
		const unsigned int nCaps = gst_caps_get_size(m_caps);
		std::vector<std::pair<double, unsigned int>> pixelrates;
		for(unsigned int n = 0; n < nCaps; ++n)
		{
			const auto capn = gst_caps_get_structure(m_caps, n);
			int width;
			int height;
			gst_structure_get_int(capn, "width", &width);
			gst_structure_get_int(capn, "height", &height);
			int framerateNominator;
			int framerateDenominator;
			gst_structure_get_fraction(capn, "framerate", &framerateNominator, &framerateDenominator);
			const auto framerate = static_cast<double>(framerateNominator)/static_cast<double>(framerateDenominator);
			if (framerate >= minimumFramerate)
			{
				const auto pixelrate = static_cast<double>(width * height) * framerate;
				pixelrates.push_back(std::make_pair(pixelrate, n));

			}
		}

		const auto highestPixelrate = std::max_element(pixelrates.begin(), pixelrates.end());
		if (highestPixelrate == pixelrates.end())
		{
			return nullptr;
		}

		auto nHighestPixelrate =(*highestPixelrate).second;
		return gst_caps_copy_nth(m_caps, guint(nHighestPixelrate));
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
