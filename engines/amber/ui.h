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

class AmberEngine;

class AmberUI {
private:
	Graphics::Surface *_frames[8];
	Graphics::Surface *_btnFrameNormal;
	Graphics::Surface *_btnFramePressed;

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
};

} // End of namespace Amber

#endif // AMBER_UI_H
