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

#include "amber/cursor.h"
#include "amber/amber.h"

namespace Amber {

AmberCursor::AmberCursor() {
}

AmberCursor::~AmberCursor() {
}

void AmberCursor::setCursor(int index, Graphics::Surface *surface, int16 hx, int16 hy) {
	if (index >= 0 && index < kNumCursors) {
		_cursors[index].surface = surface;
		_cursors[index].hotspotX = hx;
		_cursors[index].hotspotY = hy;
	}
}

void AmberCursor::setUIPalette(const byte *palette) {
	memcpy(_uiPalette, palette, 32 * 3);
}

CursorData *AmberCursor::getCursor(int index) {
	if (index < 0 || index >= kNumCursors) {
		return nullptr;
	}
	return &_cursors[index];
}

} // End of namespace Amber
