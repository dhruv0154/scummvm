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


#include "amber/font.h"

namespace Amber {


AmberFont::AmberFont() {
	for (int i = 0; i < 256; i++)
		_mappingTable[i] = 0;
	for (int i = 0; i < 94; ++i)
		_glyphs[i] = nullptr;
}

AmberFont::~AmberFont() {
	for (int i = 0; i < 94; ++i) {
		if (_glyphs[i] != nullptr) {
			_glyphs[i]->free();
			delete _glyphs[i];
			_glyphs[i] = nullptr;
		}
	}
}

void AmberFont::setMappingTable(const byte *table) {
	for (int i = 0; i < 224; i++) {
		_mappingTable[i + 32] = table[i];
	}
}

void AmberFont::setGlyph(int index, Graphics::Surface *surface) {
	if (index >= 0 && index < 94) {
		_glyphs[index] = surface;
	}
}

void AmberFont::drawString(Graphics::Screen *screen, const Common::String &text, int x, int y,
						   uint8 color, bool dropShadow, uint8 shadowColor) {
	if (!screen)
		return;

	int currentX = x;
	int currentY = y;

	// loop through every single character in the text string
	for (uint i = 0; i < text.size(); ++i) {
		// we cast to unsigned char so extended ASCII characters
		// don't turn into negative numbers and crash our array lookup
		unsigned char c = text[i];

		if (c == '\n') {
			currentX = x;  // reset X back to the starting left margin
			currentY += 6; // move Y down by 6 pixels (the height of our font)
			continue;
		}

		if (c == ' ') {
			currentX += 6; // just move the cursor forward 6 pixels since each char is 6 pixels wide
			continue;
		}

		int glyphIndex = _mappingTable[c];

		if (glyphIndex >= 0 && glyphIndex < 94) {
			if (_glyphs[glyphIndex] != nullptr) {
				Graphics::Surface *glyph = _glyphs[glyphIndex];
				// background shadow
				if (dropShadow) {
					for (int y = 0; y < glyph->h; y++) {
						const byte *srcRow = (const byte *)glyph->getBasePtr(0, y);
						byte *dstRow = (byte *)screen->getBasePtr(currentX + 1, currentY + y + 1);
						for (int gx = 0; gx < glyph->w; ++gx) {
							if (srcRow[gx] != 0) { // if it is a solid pixel
								dstRow[gx] = shadowColor;    // draw it in the passed color
							}
						}
					}
				}
				// foreground text
				for (int y = 0; y < glyph->h; y++) {
					const byte *srcRow = (const byte *)glyph->getBasePtr(0, y);
					byte *dstRow = (byte *)screen->getBasePtr(currentX, currentY + y);

					for (int gx = 0; gx < glyph->w; ++gx) {
						if (srcRow[gx] != 0) { // if it is a solid pixel
							dstRow[gx] = color;
						}
					}
				}
			}
		}

		// advance the cursor forward 6 pixels for the next letter
		currentX += 6;
	}
}

} // End of namespace Amber
