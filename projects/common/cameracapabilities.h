#ifndef CAMERACAPABILITIES_H
#define CAMERACAPABILITIES_H

#include <gst/gst.h>

struct CapabilitySelection {
	CapabilitySelection(const GstCaps* caps);
	~CapabilitySelection();
	double highestRawFrameRate() const;
	GstCaps* highestRawArea(double minimumFramerate = 0) const;
	static bool isJpeg(const GstCaps* caps);

private:
	GstCaps const* const  m_caps;
};

#endif