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

bool AmberCursor::load(const AmigaExecutable &exe, AmberEngine *engine) {
	Common::Array<Hunk> codeHunks;
	Common::Array<Hunk> dataHunks;

	for (uint i = 0; i < exe._hunks.size(); i++) {
		if (exe._hunks[i].type == HUNK_CODE)
			codeHunks.push_back(exe._hunks[i]);
		if (exe._hunks[i].type == HUNK_DATA)
			dataHunks.push_back(exe._hunks[i]);
	}

	byte *cursorBase = nullptr;

	if (dataHunks.size() == 4) {
		byte *code = codeHunks[0].data;
		uint32 codeSize = codeHunks[0].size;
		uint32 cursorOffset = 0;
		uint32 searchPos = 27500;

		for (uint32 i = searchPos; i < codeSize - 10; i++) {
			// scan for: 48 E7 00 C0 41 F9 (MOVEM.L D6/D7,-(SP) ; LEA (address), A0)
			if (code[i] == 0x48 && code[i + 1] == 0xE7 && code[i + 2] == 0x00 &&
				code[i + 3] == 0xC0 && code[i + 4] == 0x41 && code[i + 5] == 0xF9) {

				// The next 4 bytes are the actual memory address
				cursorOffset = (code[i + 6] << 24) | (code[i + 7] << 16) | (code[i + 8] << 8) | code[i + 9];
				break;
			}
		}

		if (cursorOffset > 0)
			cursorBase = dataHunks[1].data + cursorOffset;
	} else if (dataHunks.size() == 3 && codeHunks.size() > 0) {
		// cursors live exactly after the font graphics
		byte *code = codeHunks[0].data;
		uint32 codeSize = codeHunks[0].size;
		uint32 fontOffset = 0;
		uint32 searchPos = 115000;

		// skip digit font
		for (uint32 i = searchPos; i < codeSize - 10; i++) {
			if (code[i] == 0x34 && code[i + 1] == 0x3C && code[i + 2] == 0x03 &&
				code[i + 3] == 0xE7 && code[i + 4] == 0x41 && code[i + 5] == 0xF9) {
				searchPos = i + 10;
				break;
			}
		}

		searchPos += 29000;

		// find text font (22 48 41 F9)
		for (uint32 i = searchPos; i < codeSize - 8; i++) {
			if (code[i] == 0x22 && code[i + 1] == 0x48 && code[i + 2] == 0x41 && code[i + 3] == 0xF9) {
				fontOffset = (code[i + 4] << 24) | (code[i + 5] << 16) | (code[i + 6] << 8) | code[i + 7];
				break;
			}
		}

		if (fontOffset > 0) {
			// jump to the font, then skip 470 bytes (94 characters * 5 bytes each)
			cursorBase = dataHunks[1].data + fontOffset + 470;
		}
	}

	if (!cursorBase) {
		warning("Ambermoon: Failed to locate cursor graphics in executable!");
		return false;
	}

	uint32 offset = 0;
	// extract 3 bit planar cursor graphics
	for (int i = 0; i < kNumCursors; i++) {
		_cursors[i].hotspotX = (int16)((cursorBase[offset] << 8) | cursorBase[offset + 1]);
		offset += 2;

		_cursors[i].hotspotY = (int16)((cursorBase[offset] << 8) | cursorBase[offset + 1]);
		offset += 2;

		_cursors[i].surface = engine->decodePlanarGraphic(cursorBase + offset, 16, 16, 3, 24);

		// skip the image data to reach the next cursor
		// 16x16 pixels at 3 bits per pixel = 768 bits = 96 bytes
		offset += 96;
	}

	// the primary UI palette sits exactly after the cursors in memory
	// we read 32 colors, each packed into a 16-bit Amiga word (0x0RGB)
	for (int i = 0; i < 32; ++i) {
		uint16 amigaColor = (cursorBase[offset] << 8) | cursorBase[offset + 1];
		offset += 2;

		// extract the 4 bit nibbles
		uint8 r = (amigaColor >> 8) & 0x0F;
		uint8 g = (amigaColor >> 4) & 0x0F;
		uint8 b = amigaColor & 0x0F;

		// scale 4 bit (0-15) to 8 bit (0-255)
		_uiPalette[i * 3 + 0] = (r << 4) | r;
		_uiPalette[i * 3 + 1] = (g << 4) | g;
		_uiPalette[i * 3 + 2] = (b << 4) | b;
	}

	return true;
}

CursorData *AmberCursor::getCursor(int index) {
	if (index < 0 || index >= kNumCursors) {
		return nullptr;
	}
	return &_cursors[index];
}

} // End of namespace Amber
