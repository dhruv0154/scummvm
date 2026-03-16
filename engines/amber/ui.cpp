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

#include "amber/ui.h"
#include "amber/amber.h"

namespace Amber {

AmberUI::AmberUI() {
	for (int i = 0; i < 8; ++i)
		_frames[i] = nullptr;
}

AmberUI::~AmberUI() {
	for (int i = 0; i < 8; ++i) {
		if (_frames[i]) {
			_frames[i]->free();
			delete _frames[i];
		}
	}
}

bool AmberUI::load(const AmigaExecutable &exe, AmberEngine *engine) {
	// the UI graphics are in the very first data hunk
	byte *hunk0 = exe.getDataHunk(0);
	if (!hunk0)
		return false;

	// skip 160 bytes of copper commands + 32 bytes of the disable mask
	uint32 offset = 192;

	// extract the 8 border frames
	for (int i = 0; i < 8; i++) {
		// 16x16, 3 bit depth, +24 palette offset for ui
		_frames[i] = engine->decodePlanarGraphic(hunk0 + offset, 16, 16, 3, 24);
		offset += 96; // advance 96 bytes to the next frame
	}

	return true;
}

void AmberUI::drawWindow(Graphics::Screen *screen, int x, int y, int widthTiles, int heightTiles) {
	if (!screen)
		return;

	int pixelWidth = widthTiles * 16;
	int pixelHeight = heightTiles * 16;

	// draw the background fill (Color 28)
	screen->fillRect(Common::Rect(x + 16, y + 16, x + pixelWidth - 16, y + pixelHeight - 16), 28);

	// draw the corners (skipping color 24)
	screen->transBlitFrom(*_frames[FRAME_UPPER_LEFT], Common::Point(x, y), 24);
	screen->transBlitFrom(*_frames[FRAME_UPPER_RIGHT], Common::Point(x + pixelWidth - 16, y), 24);
	screen->transBlitFrom(*_frames[FRAME_LOWER_LEFT], Common::Point(x, y + pixelHeight - 16), 24);
	screen->transBlitFrom(*_frames[FRAME_LOWER_RIGHT], Common::Point(x + pixelWidth - 16, y + pixelHeight - 16), 24);

	// draw top and bottom edges
	for (int i = 1; i < widthTiles - 1; ++i) {
		screen->transBlitFrom(*_frames[FRAME_TOP], Common::Point(x + (i * 16), y), 24);
		screen->transBlitFrom(*_frames[FRAME_BOTTOM], Common::Point(x + (i * 16), y + pixelHeight - 16), 24);
	}

	// draw left and right edges
	for (int i = 1; i < heightTiles - 1; ++i) {
		screen->transBlitFrom(*_frames[FRAME_LEFT], Common::Point(x, y + (i * 16)), 24);
		screen->transBlitFrom(*_frames[FRAME_RIGHT], Common::Point(x + pixelWidth - 16, y + (i * 16)), 24);
	}
}

void AmberUI::drawBox(Graphics::Screen *screen, Common::Rect area, bool sunken) {
	if (!screen)
		return;

	byte darkColor = 26; // dark grey
	byte brightColor = 31; // white
	byte fillColor = 27; // light gray

	// swap colors based on the state
	byte topLeftColor = sunken ? darkColor : brightColor;
	byte bottomRightColor = sunken ? brightColor : darkColor;

	// upper & left Borders
	screen->hLine(area.left, area.top, area.right - 1, topLeftColor);
	screen->vLine(area.left, area.top + 1, area.bottom - 1, topLeftColor);

	// lower & right Borders
	screen->hLine(area.left + 1, area.bottom, area.right, bottomRightColor);
	screen->vLine(area.right, area.top + 1, area.bottom, bottomRightColor);

	// fill the center
	screen->fillRect(Common::Rect(area.left + 1, area.top + 1, area.right, area.bottom), fillColor);
}

} // End of namespace Amber
