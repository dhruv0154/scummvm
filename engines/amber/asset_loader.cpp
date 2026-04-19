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

#include "amber/asset_loader.h"
#include "amber/amber.h"
#include "amber/archive.h"
#include "amber/decoders.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/memstream.h"
#include "graphics/paletteman.h"


namespace Amber {

bool AmbermoonAssetLoader::ensureExeLoaded() {
	if (_exeLoaded)
		return true;

	Common::File cpuFile;
	if (!cpuFile.open("AM2_CPU")) {
		warning("AmbermoonAssetLoader: Failed to open AM2_CPU");
		return false;
	}

	if (!_exe.load(&cpuFile)) {
		warning("AmbermoonAssetLoader: Failed to parse AM2_CPU");
		return false;
	}

	_exeLoaded = true;
	return true;
}

Graphics::Surface *AmbermoonAssetLoader::decodeGlyph(byte *glyphData) {
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

bool AmbermoonAssetLoader::loadCursor(AmberEngine *engine) {
	if (!ensureExeLoaded())
		return false;

	Common::Array<Hunk> codeHunks;
	Common::Array<Hunk> dataHunks;

	for (uint i = 0; i < _exe._hunks.size(); i++) {
		if (_exe._hunks[i].type == HUNK_CODE)
			codeHunks.push_back(_exe._hunks[i]);
		if (_exe._hunks[i].type == HUNK_DATA)
			dataHunks.push_back(_exe._hunks[i]);
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
		warning("AmbermoonAssetLoader: Failed to locate cursor graphics in executable!");
		return false;
	}

	uint32 offset = 0;
	// extract 3 bit planar cursor graphics
	for (int i = 0; i < 28; i++) {
		int16 hotspotX = (int16)((cursorBase[offset] << 8) | cursorBase[offset + 1]);
		offset += 2;

		int16 hotspotY = (int16)((cursorBase[offset] << 8) | cursorBase[offset + 1]);
		offset += 2;

		Graphics::Surface *surf = engine->decodePlanarGraphic(cursorBase + offset, 16, 16, 3, 24);
		engine->_cursor->setCursor(i, surf, hotspotX, hotspotY);

		// skip the image data to reach the next cursor
		// 16x16 pixels at 3 bits per pixel = 768 bits = 96 bytes
		offset += 96;
	}

	// the primary UI palette sits exactly after the cursors in memory
	// we read 32 colors, each packed into a 16-bit Amiga word (0x0RGB)
	byte uiPalette[32 * 3];
	for (int i = 0; i < 32; ++i) {
		uint16 amigaColor = (cursorBase[offset] << 8) | cursorBase[offset + 1];
		offset += 2;

		// extract the 4 bit nibbles
		uint8 r = (amigaColor >> 8) & 0x0F;
		uint8 g = (amigaColor >> 4) & 0x0F;
		uint8 b = amigaColor & 0x0F;

		// scale 4 bit (0-15) to 8 bit (0-255)
		uiPalette[i * 3 + 0] = (r << 4) | r;
		uiPalette[i * 3 + 1] = (g << 4) | g;
		uiPalette[i * 3 + 2] = (b << 4) | b;
	}
	engine->_cursor->setUIPalette(uiPalette);

	return true;
}

bool AmbermoonAssetLoader::loadFont(AmberEngine *engine) {
	if (!ensureExeLoaded())
		return false;

	Common::Array<Hunk> codeHunks;
	Common::Array<Hunk> dataHunks;

	for (uint i = 0; i < _exe._hunks.size(); i++) {
		if (_exe._hunks[i].type == HUNK_CODE)
			codeHunks.push_back(_exe._hunks[i]);
		if (_exe._hunks[i].type == HUNK_DATA)
			dataHunks.push_back(_exe._hunks[i]);
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
	if (mappingTable) {
		engine->_font->setMappingTable(mappingTable);
	}

	// decode all 94 1 bit planar glyphs into 8 bit surfaces
	// every single character is 5 bytes long (because it is 6 pixels wide and 5 pixels tall)
	// so we jump to the correct memory offset for each character
	if (glyphBase) {
		for (int i = 0; i < 94; i++) {
			engine->_font->setGlyph(i, decodeGlyph(glyphBase + (i * 5)));
		}
	}

	return true;
}

bool AmbermoonAssetLoader::loadUI(AmberEngine *engine) {
	if (!ensureExeLoaded())
		return false;

	// the UI graphics are in the very first data hunk
	byte *hunk0 = _exe.getDataHunk(0);
	if (!hunk0)
		return false;

	// custom ui colors for blue background behind portraits and
	// for HP and SP bars
	byte customColors[24 * 3]; // 24 colors (3 bytes for each)

	// blue gradient for portraits
	for (int i = 0; i < 16; i++) {
		customColors[i * 3 + 0] = 0;      // R = 0x00
		customColors[i * 3 + 1] = 17;     // G = 0x11
		customColors[i * 3 + 2] = i * 17; // B increments by 0x11
	}

	// 216-217: HP bar
	customColors[16 * 3 + 0] = 0;
	customColors[16 * 3 + 1] = 50;
	customColors[16 * 3 + 2] = 70;
	customColors[17 * 3 + 0] = 0;
	customColors[17 * 3 + 1] = 110;
	customColors[17 * 3 + 2] = 130;

	// 218-219: SP bar
	customColors[18 * 3 + 0] = 30;
	customColors[18 * 3 + 1] = 60;
	customColors[18 * 3 + 2] = 10;
	customColors[19 * 3 + 0] = 70;
	customColors[19 * 3 + 1] = 120;
	customColors[19 * 3 + 2] = 30;

	// 220-221: text colors
	customColors[20 * 3 + 0] = 255;
	customColors[20 * 3 + 1] = 215;
	customColors[20 * 3 + 2] = 0;
	customColors[21 * 3 + 0] = 200;
	customColors[21 * 3 + 1] = 200;
	customColors[21 * 3 + 2] = 200;

	g_system->getPaletteManager()->setPalette(customColors, 200, 24);

	// skip 160 bytes of copper commands + 32 bytes of the disable mask
	uint32 offset = 192;

	// extract the 8 border frames
	for (int i = 0; i < 8; i++) {
		// 16x16, 3 bit depth, +24 palette offset for ui
		engine->_ui->setFrame(i, engine->decodePlanarGraphic(hunk0 + offset, 16, 16, 3, 24));
		offset += 96; // advance 96 bytes to the next frame
	}

	uint32 topBarOffset = 6616; // jump forward to extract portrait bar borders

	// LeftPortraitBorder (16x36, 3 planes) = 216 bytes
	Graphics::Surface *statL = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// Divider (16x36, 3 planes) = 216 bytes
	Graphics::Surface *statM = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// RightPortraitBorder (16x36, 3 planes) = 216 bytes
	Graphics::Surface *statR = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// SmallBorder1 & 2 (Combined into 32x1, 3 planes) = 12 bytes
	Graphics::Surface *statT = engine->decodePlanarGraphic(hunk0 + topBarOffset, 32, 1, 3, 56);
	Graphics::Surface *statB = engine->decodePlanarGraphic(hunk0 + topBarOffset, 32, 1, 3, 56);
	topBarOffset += 12;

	engine->_ui->setPortraitBars(statL, statM, statR, statT, statB);

	uint32 emptyFaceOffset = 11688;
	engine->_ui->setEmptyPortrait(engine->decodePlanarGraphic(hunk0 + emptyFaceOffset, 32, 34, 3, 56));

	// jump forward to extract button frames
	uint32 buttonOffset = 10336;

	// 32x17 pixels, 3 bit depth, +24 palette offset
	Graphics::Surface *btnNorm = engine->decodePlanarGraphic(hunk0 + buttonOffset, 32, 17, 3, 24);

	// advance 204 bytes for the next frame
	buttonOffset += 204;
	Graphics::Surface *btnPress = engine->decodePlanarGraphic(hunk0 + buttonOffset, 32, 17, 3, 24);

	engine->_ui->setButtonFrames(btnNorm, btnPress);

	AmberArchive archive;
	// the main UI frames are stored in Layouts.amb
	if (archive.open(Common::Path("Layouts.amb"))) {
		Common::SeekableReadStream *stream = archive.createReadStreamForMember(Common::Path("1"));

		if (stream) {
			// 3-Bit planar, +24 plaetteOffset
			engine->_ui->setExplorationLayout(engine->decodePlanarGraphic(stream, UIConstants::LAYOUT_WIDTH, UIConstants::LAYOUT_HEIGHT, 3, 24));
			delete stream;
		}
		archive.close();
	} else {
		warning("AmbermoonAssetLoader: Failed to load exploration layout from Layouts.amb!");
	}

	return true;
}

bool AmbermoonAssetLoader::loadButtons(AmberEngine *engine) {
	Common::File btnFile;
	if (!btnFile.open("Button_graphics")) {
		warning("AmbermoonAssetLoader: Failed to open Button_graphics!");
		return false;
	}

	Common::SeekableReadStream *activeStream = &btnFile;
	Common::SeekableReadStream *decryptedStream = nullptr;
	Common::SeekableReadStream *decompressedStream = nullptr;

	// decrypt (JH)
	uint32 header = activeStream->readUint32BE();
	if ((header & 0xFFFF0000) == 0x4A480000) {
		uint16 key = ((header >> 16) & 0xFFFF) ^ (header & 0xFFFF);
		decryptedStream = createJHStream(activeStream, key, btnFile.size() - 4);
		activeStream = decryptedStream;
		header = activeStream->readUint32BE(); // read the next header from the decrypted data
	}

	// decompress (LOB)
	if (header == 0x014C4F42) {
		uint32 decodedSize = activeStream->readUint32BE() & 0x00FFFFFF;
		activeStream->readUint32BE();
		decompressedStream = createLOBStream(activeStream, decodedSize);
		activeStream = decompressedStream;
	} else {
		// if it was not LOB compressed, we need to rewind the stream back 4 bytes
		// because we consumed the header but did not use it
		activeStream->seek(0);
	}

	// each button sprite is exactly 156 bytes long
	// 32 pixels wide, 13 pixels tall and 3 bit color, each plane is 52 bytes long

	activeStream->seek(76 * 156);
	engine->_ui->setCCButtonIcon(0, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));

	activeStream->seek(77 * 156);
	engine->_ui->setCCButtonIcon(1, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));

	activeStream->seek(11 * 156);
	engine->_ui->setCCButtonIcon(2, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));

	activeStream->seek(12 * 156);
	engine->_ui->setCCButtonIcon(3, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));

	activeStream->seek(28 * 156);
	engine->_ui->setCCButtonIcon(4, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));

	// the icons are 32x13 pixels, 3-bit color, each is 156 bytes
	// map our 9 grid buttons to their specific indices in the file:
	// Up-Left (8), Up (9), Up-Right (10)
	// Left (11), Wait (71), Right (12)
	// Down-Left (13), [7] Down (14), [8] Down-Right (15)
	int iconIndices[9] = {8, 9, 10, 11, 71, 12, 13, 14, 15};

	for (int i = 0; i < 9; i++) {
		activeStream->seek(iconIndices[i] * 156);
		engine->_ui->setButtonIcon(i, engine->decodePlanarGraphic(activeStream, 32, 13, 3, 24));
	}

	if (decompressedStream)
		delete decompressedStream;
	if (decryptedStream)
		delete decryptedStream;
	btnFile.close();

	return true;
}

bool AmbermoonAssetLoader::loadPartyPortraits(AmberEngine *engine) {
	AmberArchive portraitArchive;
	if (!portraitArchive.open(Common::Path("Portraits.amb"))) {
		warning("AmberUI: Portraits.amb not found");
		return false;
	}

	for (int i = 0; i < 6; i++) {
		if (engine->_party[i] != nullptr) { // If a character exists in this slot
			Common::String idStr = Common::String::format("%d", engine->_party[i]->_portraitId);
			Common::SeekableReadStream *portraitStream = portraitArchive.createReadStreamForMember(Common::Path(idStr));

			if (portraitStream) {
				Graphics::Surface *surf = engine->decodePlanarGraphic(portraitStream, 32, 34, 5, 32);
				engine->_ui->setPortrait(i, surf);
				delete portraitStream;
			}
		}
	}

	portraitArchive.close();
	return true;
}

AmberstarAssetLoader::~AmberstarAssetLoader() {
	if (_devData) {
		free(_devData);
		_devData = nullptr;
	}
}

bool AmberstarAssetLoader::ensureDevDataLoaded() {
	if (_devLoaded)
		return true;

	AmberArchive devArchive;
	// AMBERDEV.UDO is a raw LOB file
	if (!devArchive.open(Common::Path("AMBERDEV.UDO"))) {
		warning("AmberstarAssetLoader: Failed to open AMBERDEV.UDO");
		return false;
	}

	Common::SeekableReadStream *stream = devArchive.createReadStreamForMember(Common::Path("AMBERDEV.UDO"));
	if (!stream) {
		warning("AmberstarAssetLoader: Failed to decompress AMBERDEV.UDO");
		devArchive.close();
		return false;
	}

	_devDataSize = stream->size();
	_devData = (byte *)malloc(_devDataSize);
	stream->read(_devData, _devDataSize);

	delete stream;
	devArchive.close();

	_devLoaded = true;
	return true;
}

void AmberstarAssetLoader::loadUIPalette(AmberEngine *engine) {
	// hardcoded 32-byte compact UI palette
	// this represents exactly 16 colors (2 bytes per color)
	const byte hardcodedPalette[] = {
		0x00, 0x00, 0x07, 0x50, 0x03, 0x33, 0x02, 0x22,
		0x01, 0x11, 0x07, 0x42, 0x06, 0x31, 0x02, 0x00,
		0x05, 0x66, 0x03, 0x45, 0x07, 0x54, 0x06, 0x43,
		0x05, 0x32, 0x04, 0x21, 0x03, 0x10, 0x07, 0x65};

	Common::MemoryReadStream paletteStream(hardcodedPalette, sizeof(hardcodedPalette));

	// create an array for the decoded 8-bit RGB values (16 colors * 3 channels = 48 bytes)
	byte uiPalette[16 * 3];

	decodeCompactPalette(&paletteStream, uiPalette);
	engine->_cursor->setUIPalette(uiPalette);
}

Graphics::Surface *AmberstarAssetLoader::decodeAmberstarGlyph(byte *glyphData) {
	Graphics::Surface *surface = new Graphics::Surface();

	// amberstar glyphs are exactly 8 pixels wide and 5 pixels tall (1 bit per pixel)
	// we create an 8x6 canvas, leaving the bottom row empty
	// so the text has vertical line spacing
	surface->create(8, 6, Graphics::PixelFormat::createFormatCLUT8());

	for (int y = 0; y < 6; y++) {
		byte *row = (byte *)surface->getBasePtr(0, y);

		// the 6th row (index 5) is spacing, fill it with 0 (transparent)
		if (y == 5) {
			for (int x = 0; x < 8; ++x) {
				row[x] = 0;
			}
			continue;
		}

		// read the single byte that represents this entire row of 8 pixels
		byte line = glyphData[y];

		for (int x = 0; x < 8; ++x) {
			// check each bit from left (MSB) to right (LSB).
			// If the bit is 1, draw color 15 (white text)
			// If the bit is 0, draw color 0 (transparent)
			if (line & (1 << (7 - x))) {
				row[x] = 15;
			} else {
				row[x] = 0;
			}
		}
	}
	return surface;
}

bool AmberstarAssetLoader::loadCursor(AmberEngine *engine) {
	return true;
}
bool AmberstarAssetLoader::loadFont(AmberEngine *engine) {
	if (!ensureDevDataLoaded())
		return false;

	// the exact 16-byte signature of the Text_conversion_tab from the asm
	const byte fontSignature[] = {
		0x3A, 0x24, 0x23, 0x2C, 0xFF, 0x2A, 0xFF, 0x22,
		0x28, 0x29, 0x26, 0x2E, 0x20, 0x2D, 0x21, 0x2B};

	uint32 fontOffset = 0;
	bool found = false;

	// scan the LOB decompressed data of AMBERDEV.UDO for the signature
	for (uint32 i = 0; i < _devDataSize - sizeof(fontSignature); i++) {
		bool match = true;
		for (uint32 j = 0; j < sizeof(fontSignature); j++) {
			if (_devData[i + j] != fontSignature[j]) {
				match = false;
				break;
			}
		}

		if (match) {
			fontOffset = i;
			found = true;
			break;
		}
	}

	if (!found) {
		warning("AmberstarAssetLoader: Could not find font mapping signature in AMBERDEV.UDO!");
		return false;
	}

	// read the text conversion table (224 bytes)
	engine->_font->setMappingTable(_devData + fontOffset);

	// skip the 224-byte text mapping and the 224-byte rune mapping to reach the pixels
	uint32 glyphDataOffset = fontOffset + 224 + 224;

	// decode the glyphs
	// there are 89 glyphs in amberstar (calculated from Pyrdacor's 445 byte total)
	for (int i = 0; i < 89; i++) {
		// calculate the exact memory address for this specific letter
		byte *currentGlyphData = _devData + glyphDataOffset + (i * 5);

		// decode it using our 1-bit planar glyph decoder and push it to the engine
		Graphics::Surface *glyphSurface = decodeAmberstarGlyph(currentGlyphData);
		engine->_font->setGlyph(i, glyphSurface);
	}

	return true;
}
uint32 AmberstarAssetLoader::findSignature(const byte *data, uint32 dataSize, const byte *signature, uint32 sigSize) {
	if (sigSize == 0 || dataSize < sigSize)
		return 0;

	for (uint32 i = 0; i <= dataSize - sigSize; i++) {
		bool match = true;
		for (uint32 j = 0; j < sigSize; j++) {
			if (data[i + j] != signature[j]) {
				match = false;
				break;
			}
		}
		if (match)
			return i;
	}
	return 0;
}

bool AmberstarAssetLoader::loadUI(AmberEngine *engine) {
	loadUIPalette(engine);

	if (!ensureDevDataLoaded())
		return false;

	byte customColors[24 * 3]; // 24 colors total
	for (int i = 0; i < 16; i++) {
		customColors[i * 3 + 0] = i * 10; // R fades from black (0) to blood red (150)
		customColors[i * 3 + 1] = 0;      // G
		customColors[i * 3 + 2] = 0;      // B
	}

	// 216-217: Amberstar HP bar (Shadow, Main)
	customColors[16 * 3 + 0] = 120; // Shadow R (Dark Brown)
	customColors[16 * 3 + 1] = 50;  // Shadow G
	customColors[16 * 3 + 2] = 10;  // Shadow B
	customColors[17 * 3 + 0] = 230; // Main R (Golden Orange)
	customColors[17 * 3 + 1] = 140; // Main G
	customColors[17 * 3 + 2] = 20;  // Main B

	// 218-219: Amberstar SP bar (Shadow, Main)
	customColors[18 * 3 + 0] = 20;  // Shadow R (Dark Blue)
	customColors[18 * 3 + 1] = 40;  // Shadow G
	customColors[18 * 3 + 2] = 120; // Shadow B
	customColors[19 * 3 + 0] = 50;  // Main R (Light Blue)
	customColors[19 * 3 + 1] = 120; // Main G
	customColors[19 * 3 + 2] = 200; // Main B

	g_system->getPaletteManager()->setPalette(customColors, 200, 24);

	// Extracted from ST source: SB_LEFT.IMG
	const byte sigLeftEdge[] = {
		0x00, 0x1c, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x30, 0x00, 0xce, 0x00, 0x00};

	// Extracted from ST source: SB_MID.IMG
	const byte sigMidEdge[] = {
		0x65, 0x5c, 0x9b, 0xbf, 0x00, 0x00, 0x64, 0x40, 0xa6, 0x49, 0xbe, 0x79, 0x41, 0x86, 0x00, 0x00};

	// Extracted from ST source: SB_RIGHT.IMG
	const byte sigRightEdge[] = {
		0xa0, 0x00, 0xe0, 0x00, 0x18, 0x00, 0x00, 0x00, 0xa4, 0x00, 0xbc, 0x00, 0x43, 0x00, 0x00, 0x00};

	// Extracted from ST source: SB_TOP.IMG
	const byte sigTopEdge[] = {
		0xe6, 0x9e, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x80, 0x6a, 0xa6, 0xf7, 0xdf, 0x00, 0x00, 0x08, 0x20};

	// Extracted from ST source: SB_BOTT.IMG
	const byte sigBotEdge[] = {
		0xd8, 0x30, 0xd8, 0x30, 0x27, 0xcf, 0x00, 0x00, 0xdb, 0x86, 0xdb, 0x86, 0x24, 0x79, 0x00, 0x00};

	const byte sigFrame001[] = {
		0x08, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x4f, 0x0d, 0x0e, 0x0e};

	// LAYOUT.ICN (16x16 tile graphics)
	const byte sigLayoutIcn[] = {
		0xaa, 0xaa, 0x00, 0x00, 0xaa, 0xaa, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00};

	// look for the offsets in AMBERDEV.UDO
	uint32 offsetL = findSignature(_devData, _devDataSize, sigLeftEdge, sizeof(sigLeftEdge));
	uint32 offsetM = findSignature(_devData, _devDataSize, sigMidEdge, sizeof(sigMidEdge));
	uint32 offsetR = findSignature(_devData, _devDataSize, sigRightEdge, sizeof(sigRightEdge));
	uint32 offsetT = findSignature(_devData, _devDataSize, sigTopEdge, sizeof(sigTopEdge));
	uint32 offsetB = findSignature(_devData, _devDataSize, sigBotEdge, sizeof(sigBotEdge));

	// look for layout signatures
	uint32 offsetLayouts = findSignature(_devData, _devDataSize, sigFrame001, sizeof(sigFrame001));
	uint32 offsetLayoutIcn = findSignature(_devData, _devDataSize, sigLayoutIcn, sizeof(sigLayoutIcn));

	Graphics::Surface *statL = nullptr, *statM = nullptr, *statR = nullptr, *statT = nullptr, *statB = nullptr;

	if (offsetL)
		statL = engine->decodePlanarGraphic(_devData + offsetL, 16, 36, 4, 0, true);
	if (offsetM)
		statM = engine->decodePlanarGraphic(_devData + offsetM, 16, 36, 4, 0, true);
	if (offsetR)
		statR = engine->decodePlanarGraphic(_devData + offsetR, 16, 36, 4, 0, true);
	if (offsetT)
		statT = engine->decodePlanarGraphic(_devData + offsetT, 32, 1, 4, 0, true);
	if (offsetB)
		statB = engine->decodePlanarGraphic(_devData + offsetB, 32, 1, 4, 0, true);

	engine->_ui->setPortraitBars(statL, statM, statR, statT, statB);

	// parse FRAME.001 and build the exploration layout
	if (offsetLayouts && offsetLayoutIcn) {
		Graphics::Surface *layoutSurf = new Graphics::Surface();
		layoutSurf->create(320, 163, Graphics::PixelFormat::createFormatCLUT8());

		byte *frameData = _devData + offsetLayouts;
		int destY = 0;
		int frameIdx = 0;

		// 1st line (height 12, cropped 4 pixels from top)
		for (int x = 0; x < 20; x++) {
			uint8 iconId = frameData[frameIdx++] - 1; // 1 based indices
			Graphics::Surface *tile = engine->decodePlanarGraphic(_devData + offsetLayoutIcn + (iconId * 128), 16, 16, 4, 0, true);
			for (int ty = 4; ty < 16; ty++) {
				memcpy(layoutSurf->getBasePtr(x * 16, destY + ty - 4), tile->getBasePtr(0, ty), 16);
			}
			tile->free();
			delete tile;
		}
		destY += 12;

		// 9 middle lines (full height 16)
		for (int y = 0; y < 9; y++) {
			for (int x = 0; x < 20; x++) {
				uint8 iconId = frameData[frameIdx++] - 1;
				Graphics::Surface *tile = engine->decodePlanarGraphic(_devData + offsetLayoutIcn + (iconId * 128), 16, 16, 4, 0, true);
				for (int ty = 0; ty < 16; ty++) {
					memcpy(layoutSurf->getBasePtr(x * 16, destY + ty), tile->getBasePtr(0, ty), 16);
				}
				tile->free();
				delete tile;
			}
			destY += 16;
		}

		// last line (height 7, cropped from bottom)
		for (int x = 0; x < 20; x++) {
			uint8 iconId = frameData[frameIdx++] - 1;
			Graphics::Surface *tile = engine->decodePlanarGraphic(_devData + offsetLayoutIcn + (iconId * 128), 16, 16, 4, 0, true);
			for (int ty = 0; ty < 7; ty++) {
				memcpy(layoutSurf->getBasePtr(x * 16, destY + ty), tile->getBasePtr(0, ty), 16);
			}
			tile->free();
			delete tile;
		}

		engine->_ui->setExplorationLayout(layoutSurf);

		// decode amberstone icon for empty party slots in the portrait bar
		Graphics::Surface *emptyPortraitSurf = nullptr;
		uint32 offsetAmberstone = 261892;

		if (_devDataSize >= offsetAmberstone + 544)
			emptyPortraitSurf = engine->decodePlanarGraphic(_devData + offsetAmberstone, 32, 34, 4, 0, true);

		engine->_ui->setEmptyPortrait(emptyPortraitSurf);

	} else {
		warning("AmberstarAssetLoader: Could not find FRAME.001 or LAYOUT.ICN signatures in AMBERDEV.UDO!");
	}

	return true;
}

bool AmberstarAssetLoader::loadButtons(AmberEngine *engine) {
	if (!ensureDevDataLoaded())
		return false;

	const byte sigControlIcn[] = {
		0x3f, 0xff, 0xff, 0xff, 0x3f, 0xff, 0x3f, 0xff, 0xff, 0xf8, 0xff, 0xff, 0xff, 0xf8, 0xff, 0xf8};

	uint32 offsetControl = findSignature(_devData, _devDataSize, sigControlIcn, sizeof(sigControlIcn));

	if (offsetControl) {
		uint32 bytesPerIcon = 256;
		// 2D city movement indices mapped from C2_CIL in the asm
		int iconIndices[9] = {46, 1, 45, 4, 7, 3, 48, 2, 47};

		for (int i = 0; i < 9; i++) {
			// subtract 1 because index 0 is an empty block preceding CONTROL.ICN
			int actualIdx = iconIndices[i] - 1;
			Graphics::Surface *btn = engine->decodePlanarGraphic(_devData + offsetControl + (actualIdx * bytesPerIcon), 32, 16, 4, 0, true);
			engine->_ui->setButtonIcon(i, btn);
		}
	} else {
		warning("AmberstarAssetLoader: CONTROL.ICN signature not found in AMBERDEV.UDO!");
	}
	return true;
}

bool AmberstarAssetLoader::loadPartyPortraits(AmberEngine *engine) {
	AmberArchive charArchive;

	// CHARDATA.AMB is an uncompressed AMBR archive
	if (!charArchive.open(Common::Path("CHARDATA.AMB"))) {
		warning("AmberstarAssetLoader: CHARDATA.AMB not found!");
		return false;
	}

	for (int i = 0; i < 6; i++) {
		if (engine->_party[i] != nullptr) {
			Common::String idStr = Common::String::format("%d", engine->_party[i]->_portraitId);
			Common::SeekableReadStream *charStream = charArchive.createReadStreamForMember(Common::Path(idStr));

			if (charStream) {
				// CHARDATA.AMB portraits start at 0x06AA (1706)
				charStream->seek(0x06AA);

				// Amberstar CHARDATA portraits DO have a 6-byte header
				uint16 width_m1 = charStream->readUint16BE();
				uint16 height_m1 = charStream->readUint16BE();
				uint16 numBitplanes = charStream->readUint16BE();

				// If width is 0, the character has no portrait
				if (width_m1 == 0 && height_m1 == 0) {
					delete charStream;
					continue;
				}

				uint16 actualWidth = width_m1 + 1;
				uint16 actualHeight = height_m1 + 1;

				// Palette offset is 0 (uses the UI palette we loaded earlier)
				Graphics::Surface *surf = engine->decodePlanarGraphic(charStream, actualWidth, actualHeight, numBitplanes, 0, true);

				engine->_ui->setPortrait(i, surf);

				delete charStream;
			}
		}
	}

	charArchive.close();
	return true;
}

} // End of namespace Amber
