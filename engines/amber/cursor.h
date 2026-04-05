/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef AMBER_CURSOR_H
#define AMBER_CURSOR_H

#include "common/scummsys.h"
#include "graphics/screen.h"
#include "amiga.h"

namespace Amber {

struct CursorData {
	int16 hotspotX;
	int16 hotspotY;
	Graphics::Surface *surface;

	CursorData() : hotspotX(0), hotspotY(0), surface(nullptr) {}

	~CursorData() {
		if (surface) {
			surface->free();
			delete surface;
			surface = nullptr;
		}
	}
};

class AmberEngine;

class AmberCursor {
private:
	// ambermoon has exactly 28 cursors
	static const int kNumCursors = 28;

	CursorData _cursors[kNumCursors];

	byte _uiPalette[32 * 3];

public:
	AmberCursor();
	~AmberCursor();

	void setCursor(int index, Graphics::Surface *surface, int16 hx, int16 hy);
	void setUIPalette(const byte *palette);

	CursorData *getCursor(int index);

	const byte *getUIPalette() const { return _uiPalette; }
};

} // End of namespace Amber

#endif // AMBER_CURSOR_H
