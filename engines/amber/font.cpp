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

Graphics::Surface* AmberFont::decodeGlyph(byte* glyphData) {
	Graphics::Surface *surface = new Graphics::Surface();

	// create a 6x6 canvas for the letter
	// (the letter is 6x5, but we leave the 6th row empty for spacing)
	surface->create(6, 6, Graphics::PixelFormat::createFormatCLUT8());

	// loop through the 6 rows (y)
	for (int y = 0; y < 6; y++) {
		byte *row = (byte *)surface->getBasePtr(0, y);

		// if it is the 6th row (index 5), fill with 0 (transparent) and skip
		if (y == 5) {
			for (int x = 0; x < 6; ++x)
				row[x] = 0;
			continue;
		}

		// read the 1 byte for this specific row of the letter
		byte line = glyphData[y];

		// loop through the 6 pixels (x)
		for (int x = 0; x < 6; ++x) {
			// we check if the leftmost bit is a 1
			if (line & 0x80) {
				row[x] = 15; // color 15 (white text)
			} else {
				row[x] = 0; // color 0 (transparent)
			}

			line <<= 1;
		}
	}
	return surface;
}

bool AmberFont::load(const AmigaExecutable &exe) {
	Common::Array<Hunk> codeHunks;
	Common::Array<Hunk> dataHunks;

	for (uint i = 0; i < exe._hunks.size(); i++) {
		if (exe._hunks[i].type == HUNK_CODE)
			codeHunks.push_back(exe._hunks[i]);
		if (exe._hunks[i].type == HUNK_DATA)
			dataHunks.push_back(exe._hunks[i]);
	}
	byte *glyphBase = nullptr;
	byte *mappingTable = nullptr;

	// new releases of ambermoon modified the game's
	// file structure to make it easier to edit fonts
	// they moved font into it's own brand new 4th data hunk
	if (dataHunks.size() == 4) {
		mappingTable = dataHunks[3].data;
		glyphBase = dataHunks[3].data + 448;
	} else if (dataHunks.size() == 3 && codeHunks.size() > 0) {
		byte *code = codeHunks[0].data;
		uint32 codeSize = codeHunks[0].size;
		uint32 fontOffset = 0;
		uint32 searchPos = 115000; // we start at 115000 to skip unrelated game logic

		// ambermoon has two fonts digit font (for tiny numbers) and text font (for normal reading)
		// we are looking for text font
		for (uint32 i = searchPos; i < codeSize - 10; i++) {
			// this hex sequence is the asm code that loads digit font
			// we find it and skip it
			if (code[i] == 0x34 && code[i + 1] == 0x3C && code[i + 2] == 0x03 &&
				code[i + 3] == 0xE7 && code[i + 4] == 0x41 && code[i + 5] == 0xF9) {
				searchPos = i + 10;
				break;
			}
		}
		// Pyrdacor's C# version shows that the routine to load the text font
		// is exactly 29000 bytes further down in the code from digit font routine
		searchPos += 29000;

		for (uint32 i = searchPos; i < codeSize - 8; i++) {
			// the next 4 bytes are for text font
			if (code[i] == 0x22 && code[i + 1] == 0x48 && code[i + 2] == 0x41 && code[i + 3] == 0xF9) {
				// convert the number from big endian to little endian
				fontOffset = (code[i + 4] << 24) | (code[i + 5] << 16) | (code[i + 6] << 8) | code[i + 7];
				break;
			}
		}
		// add the offset to start of data hunk 2 to get the exact location of our one bit graphics
		if (fontOffset > 0)
			glyphBase = dataHunks[1].data + fontOffset;

		// we have the graphics but do not know which graphics is 'A' or 'B'
		// the games uses a 448 bytes mapping table to tell which graphics
		// belongs to which ASCII char, here we find that table using the 10 byte signature
		// which is a specific sequence which occurs right after the table
		byte *data = dataHunks[1].data;
		uint32 dataSize = dataHunks[1].size;
		const byte signature[] = {0x03, 0x00, 0x1C, 0x1C, 0x1A, 0x1A, 0x19, 0x1B, 0x18, 0x18};

		// slide a window of 10 across data hunk 2 to find our signature
		for (uint32 i = 448; i < dataSize - 10; i++) {
			bool match = true;
			for (int j = 0; j < 10; j++) {
				if (data[i + j] != signature[j]) {
					match = false;
					break;
				}
			}

			// if we found the signature, the mapping table starts exactly 448 bytes before it
			if (match) {
				mappingTable = data + (i - 448);
				break;
			}
		}
	}

	// The table maps ASCII 32 to 255, we align it perfectly in our _mappingTable
	for (int i = 0; i < 224; i++)
		_mappingTable[i + 32] = mappingTable[i];

	// decode all 94 1 bit planar glyphs into 8 bit surfaces
	// every single character is 5 bytes long (because it is 6 pixels wide and 5 pixels tall)
	// so we jump to the correct memory offset for each character
	for (int i = 0; i < 94; i++)
		_glyphs[i] = decodeGlyph(glyphBase + (i * 5));

	return true;
}

void AmberFont::drawString(Graphics::Screen *screen, const Common::String &text, int x, int y) {
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
				for (int y = 0; y < glyph->h; y++) {
					const byte *srcRow = (const byte *)glyph->getBasePtr(0, y);
					byte *dstRow = (byte *)screen->getBasePtr(currentX + 1, currentY + y + 1);
					for (int gx = 0; gx < glyph->w; ++gx) {
						if (srcRow[gx] != 0) { // if it is a solid pixel
							dstRow[gx] = 0;    // draw it black for the shadow
						}
					}
				}
				// foreground text
				for (int y = 0; y < glyph->h; y++) {
					const byte *srcRow = (const byte *)glyph->getBasePtr(0, y);
					byte *dstRow = (byte *)screen->getBasePtr(currentX, currentY + y);

					for (int gx = 0; gx < glyph->w; ++gx) {
						if (srcRow[gx] != 0) { // if it is a solid pixel
							dstRow[gx] = 31;   // color 31 is white in the UI palette
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
