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

#ifndef AMBER_CHARACTER_CREATOR_H
#define AMBER_CHARACTER_CREATOR_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/rect.h"
#include "common/events.h"
#include "graphics/surface.h"

namespace Amber {
class AmberEngine;

struct UIHitbox {
	int id;
	Common::Rect rect;
};

class CharacterCreator {
public:
	CharacterCreator(AmberEngine *engine);

	~CharacterCreator();

	void execute();

private:
	AmberEngine *_engine;

	Common::String _playerName;
	int _pressedButtonId; // tracks which button is currently held down (-1 for none)
	bool _isFemale;
	int _portraitListIndex; // tracks which of the 4 portraits is currently selected

	int _winX;
	int _winY;

	Graphics::Surface *_iconMale;
	Graphics::Surface *_iconFemale;
	Graphics::Surface *_iconLeft;
	Graphics::Surface *_iconRight;
	Graphics::Surface *_iconOk;

	UIHitbox _buttons[5];

	static const int _malePortraitIds[4];
	static const int _femalePortraitIds[4];

	// extracts the 5 button icons from Button_graphics into our _icon variables
	void loadAssets();

	// processes a single mouse or keyboard event
	void handleEvent(const Common::Event &e, const Common::Point &mousePos);

	void draw();

	// function to extract and draw the currently selected portrait
	void drawPortrait();
};


} // End of namespace Amber

#endif // AMBER_CHARACTER_CREATOR_H
