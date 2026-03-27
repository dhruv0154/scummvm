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
#include "amber/archive.h"
#include "graphics/paletteman.h"

namespace Amber {

AmberUI::AmberUI() {
	for (int i = 0; i < 8; ++i)
		_frames[i] = nullptr;
	for (int i = 0; i < 6; ++i)
		_portraits[i] = nullptr;
	_btnFrameNormal = nullptr;
	_btnFramePressed = nullptr;
	_explorationLayout = nullptr;
	_statusL = nullptr;
	_statusM = nullptr;
	_statusR = nullptr;
	_statusTB = nullptr;
	_emptyPortrait = nullptr;
}

AmberUI::~AmberUI() {
	for (int i = 0; i < 8; ++i) {
		if (_frames[i]) {
			_frames[i]->free();
			delete _frames[i];
		}
	}
	for (int i = 0; i < 6; ++i) {
		if (_portraits[i]) {
			_portraits[i]->free();
			delete _portraits[i];
		}
	}
	if (_btnFrameNormal) {
		_btnFrameNormal->free();
		delete _btnFrameNormal;
	}
	if (_explorationLayout) {
		_explorationLayout->free();
		delete _explorationLayout;
	}
	if (_statusL) {
		_statusL->free();
		delete _statusL;
	}
	if (_statusM) {
		_statusM->free();
		delete _statusM;
	}
	if (_statusR) {
		_statusR->free();
		delete _statusR;
	}
	if (_statusTB) {
		_statusTB->free();
		delete _statusTB;
	}
	if (_emptyPortrait) {
		_emptyPortrait->free();
		delete _emptyPortrait;
	}
}

bool AmberUI::load(const AmigaExecutable &exe, AmberEngine *engine) {
	// the UI graphics are in the very first data hunk
	byte *hunk0 = exe.getDataHunk(0);
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
		_frames[i] = engine->decodePlanarGraphic(hunk0 + offset, 16, 16, 3, 24);
		offset += 96; // advance 96 bytes to the next frame
	}

	uint32 topBarOffset = 6616; // jump forward to extract portrait bar borders

	// LeftPortraitBorder (16x36, 3 planes) = 216 bytes
	_statusL = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// Divider (16x36, 3 planes) = 216 bytes
	_statusM = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// RightPortraitBorder (16x36, 3 planes) = 216 bytes
	_statusR = engine->decodePlanarGraphic(hunk0 + topBarOffset, 16, 36, 3, 56);
	topBarOffset += 216;

	// SmallBorder1 & 2 (Combined into 32x1, 3 planes) = 12 bytes
	_statusTB = engine->decodePlanarGraphic(hunk0 + topBarOffset, 32, 1, 3, 56);
	topBarOffset += 12;

	uint32 emptyFaceOffset = 11688;
	_emptyPortrait = engine->decodePlanarGraphic(hunk0 + emptyFaceOffset, 32, 34, 3, 56);

	// jump forward to extract button frames
	uint32 buttonOffset = 10336;

	// 32x17 pixels, 3 bit depth, +24 palette offset
	_btnFrameNormal = engine->decodePlanarGraphic(hunk0 + buttonOffset, 32, 17, 3, 24);

	// advance 204 bytes for the next frame
	buttonOffset += 204;
	_btnFramePressed = engine->decodePlanarGraphic(hunk0 + buttonOffset, 32, 17, 3, 24);

	return true;
}

void AmberUI::drawPortraitBar(Graphics::Screen *screen, AmberEngine *engine) {
	if (!screen || !_statusL)
		return;

	// draw left edge
	screen->transBlitFrom(*_statusL, Common::Point(0, 0), 56);

	// draw the 6 middle dividers
	int currentX = 48;
	for (int i = 0; i < 6; i++) {
		screen->transBlitFrom(*_statusM, Common::Point(currentX, 0), 56);
		currentX += 48;
	}

	// draw right edge
	screen->transBlitFrom(*_statusR, Common::Point(currentX - 32, 0), 56);

	// draw the 6 portrait slots
	for (int slot = 0; slot < 6; slot++) {
		int portraitX = (slot * 48) + 16;

		screen->transBlitFrom(*_statusTB, Common::Point(portraitX, 0), 56);
		screen->transBlitFrom(*_statusTB, Common::Point(portraitX, 35), 56);

		AmberPerson *person = engine->_party[slot];

		if (person && _portraits[slot]) {

			drawPortraitBackground(screen, portraitX, 1);

			// transparency key is 57 (mask color 25 + palette offset 32)
			screen->transBlitFrom(*_portraits[slot], Common::Point(portraitX, 1), 57);

			// draw HP bar
			drawBar(screen, portraitX + 35, 19, 3, person->_currentHP, person->_maxHP, 217, 216);

			// draw SP bar for characters with magic
			if (person->_hasMagic)
				drawBar(screen, portraitX + 43, 19, 3, person->_currentSP, person->_maxSP, 219, 218);

			// draw character name
			Common::String shortName = person->_name.substr(0, 5);

			// 220 = yellow (active), 221 = white (inactive)
			uint8 nameColor = (slot == 0) ? 220 : 221;

			engine->_font->drawString(screen, shortName, portraitX + 2, 28, nameColor);

		} else if (_emptyPortrait) {
			screen->transBlitFrom(*_emptyPortrait, Common::Point(portraitX, 1), 56);
		}
	}
}

bool AmberUI::loadPartyPortraits(AmberEngine *engine) {
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
				_portraits[i] = engine->decodePlanarGraphic(portraitStream, 32, 34, 5, 32);
				delete portraitStream;
			}
		}
	}

	portraitArchive.close();
	return true;
}

void AmberUI::drawPortraitBackground(Graphics::Screen *screen, int x, int y) {
	for (int i = 0; i < 34; i++) {
		// calculate the gradient index (changes every 2 pixels after row 4)
		int colorIndex = 0;
		if (i >= 4) {
			colorIndex = (i - 4) / 2 + 1;
			if (colorIndex > 15)
				colorIndex = 15;
		}
		// draw using our custom blue palette (indices 200-215)
		screen->hLine(x, y + i, x + 31, 200 + colorIndex);
	}
}

void AmberUI::drawBar(Graphics::Screen *screen, int x, int y, int w, int current, int max, byte color, byte shadowColor) {
	if (max <= 0)
		return;

	int maxHeight = 16;
	if (current > max)
		current = max;
	if (current < 0)
		current = 0;

	int fillHeight = (current * maxHeight) / max;
	int emptyHeight = maxHeight - fillHeight;

	if (fillHeight > 0) {
		// draw the shadow column only on the filled portion
		screen->vLine(x - 1, y + emptyHeight, y + maxHeight - 1, shadowColor);
		// draw the filled portion
		screen->fillRect(Common::Rect(x, y + emptyHeight, x + w, y + maxHeight), color);
	}
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

void AmberUI::drawButton(Graphics::Screen *screen, int x, int y, bool pressed) {
	if (!screen)
		return;

	Graphics::Surface *frame = pressed ? _btnFramePressed : _btnFrameNormal;
	if (frame)
		screen->transBlitFrom(*frame, Common::Point(x, y), 24);
}

bool AmberUI::loadExplorationLayout(AmberEngine *engine) {
	AmberArchive archive;

	// the main UI frames are stored in Layouts.amb
	if (archive.open(Common::Path("Layouts.amb"))) {
		Common::SeekableReadStream *stream = archive.createReadStreamForMember(Common::Path("1"));

		if (stream) {
			// 3-Bit planar, +24 plaetteOffset
			_explorationLayout = engine->decodePlanarGraphic(stream, UIConstants::LAYOUT_WIDTH, UIConstants::LAYOUT_HEIGHT, 3, 24);
			delete stream;
			archive.close();
			return true;
		}
		archive.close();
	}

	warning("AmberUI: Failed to load exploration layout from Layouts.amb!");
	return false;
}

void AmberUI::drawExplorationLayout(Graphics::Screen *screen) {
	if (screen && _explorationLayout)
		// we use 24 as the transparency key because the +24 offset shifted the empty pixels
		screen->transBlitFrom(*_explorationLayout, Common::Point(UIConstants::LAYOUT_X, UIConstants::LAYOUT_Y), 24);
}

} // End of namespace Amber
