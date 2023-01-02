#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <string>
#include <vector>

#include "gst/gst.h"

struct ElementSelection {
	ElementSelection(const std::vector<std::string>& candidates, const std::string& name);
	std::string elementName() const;
	GstElement* element();

private:
	GstElement* m_element;
};

#endif

