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

#ifndef AMBER_FONT_H
#define AMBER_FONT_H

#include "common/scummsys.h"
#include "common/str.h"
#include "graphics/surface.h"
#include "graphics/screen.h"
#include "amber/amiga.h"

namespace Amber {

class AmberFont {
private:
	// ambermoon has exactly 94 unique character graphics
	Graphics::Surface *_glyphs[94];

	// characters do not map to their ASCII index, they map to a custom index
	// store a copy of the game's mapping table here to translate ASCII to glyph index
	byte _mappingTable[256];

public:
	AmberFont();
	~AmberFont();

	void setMappingTable(const byte *table);
	void setGlyph(int index, Graphics::Surface *surface);

	void drawString(Graphics::Screen *screen, const Common::String &text, int x, int y, uint8 color = 31,
					bool dropShadow = true, uint8 shadowColor = 0);
};

} // End of namespace Amber

#endif // AMBER_FONT_H
