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

#ifndef AMBER_UI_H
#define AMBER_UI_H

#include "amber/amiga.h"
#include "common/scummsys.h"
#include "graphics/screen.h"

namespace Amber {

namespace UIConstants {
// map viewport
const int MAP_VIEW_X = 16;
const int MAP_VIEW_Y = 49;
const int MAP_VIEW_WIDTH = 176;  // 11 tiles * 16px
const int MAP_VIEW_HEIGHT = 144; // 9 tiles * 16px

// layout dimensions
const int LAYOUT_X = 0;
const int LAYOUT_Y = 37;
const int LAYOUT_WIDTH = 320;
const int LAYOUT_HEIGHT = 163;

// movement pad grid starting position
const int BUTTON_GRID_X = 208;
const int BUTTON_GRID_Y = 143;
const int BUTTON_WIDTH = 32;
const int BUTTON_HEIGHT = 17;
} // End of namespace UIConstants

class AmberEngine;

class AmberUI {
private:
	Graphics::Surface *_frames[8];
	Graphics::Surface *_btnFrameNormal;
	Graphics::Surface *_btnFramePressed;
	Graphics::Surface *_explorationLayout;

	Graphics::Surface *_statusL;
	Graphics::Surface *_statusM;
	Graphics::Surface *_statusR;
	Graphics::Surface *_statusTB;
	Graphics::Surface *_emptyPortrait;

	Graphics::Surface *_portraits[6];

	enum FrameType {
		FRAME_UPPER_LEFT,
		FRAME_LEFT,
		FRAME_LOWER_LEFT,
		FRAME_TOP,
		FRAME_BOTTOM,
		FRAME_UPPER_RIGHT,
		FRAME_RIGHT,
		FRAME_LOWER_RIGHT
	};

public:
	AmberUI();
	~AmberUI();

	bool load(const AmigaExecutable &exe, AmberEngine *engine);

	// draws window with the stone borders and colored background
	void drawWindow(Graphics::Screen *screen, int x, int y, int widthTiles, int heightTiles);

	// draws the 3D boxes used for buttons and text inputs
	void drawBox(Graphics::Screen *screen, Common::Rect area, bool sunken);
	void drawButton(Graphics::Screen *screen, int x, int y, bool pressed = false);

	// load and draw the main game ui
	bool loadExplorationLayout(AmberEngine *engine);
	void drawExplorationLayout(Graphics::Screen *screen);

	void drawPortraitBar(Graphics::Screen *screen, AmberEngine *engine);
	bool loadPartyPortraits(AmberEngine *engine);

	void drawPortraitBackground(Graphics::Screen *screen, int x, int y);
	void drawBar(Graphics::Screen *screen, int x, int y, int w, int current, int max, byte color, byte shadowColor);
	};

} // End of namespace Amber

#endif // AMBER_UI_H
