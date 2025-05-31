// Implementation of Hotspots class from hotspots.h
#include "common/system.h"
#include "graphics/surface.h"

#include "gui/hotspots.h"

namespace GUI {

MgoHotspots::MgoHotspots() {
    // Constructor implementation (initialize as needed)
	_surface = new Graphics::Surface();
	Graphics::PixelFormat overlayFormat = g_system->getOverlayFormat();
	_surface->create(10, 10, overlayFormat);
}

MgoHotspots::~MgoHotspots() {
    // Destructor implementation (cleanup as needed)
	delete _surface;
}

void MgoHotspots::drawHotspots(const std::vector<Common::Rect>& points) {
    // Placeholder: Implement drawing logic for hotspots here
    // For example, iterate and draw each point
    for (const auto& pt : points) {
		g_system->clearOverlay();
		g_system->copyRectToOverlay((byte *)_surface->getPixels(), _surface->pitch,
				pt.left, pt.top, pt.width(), pt.height());
    }
}

} // namespace Mgo
