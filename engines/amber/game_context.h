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

#ifndef AMBER_GAME_CONTEXT_H
#define AMBER_GAME_CONTEXT_H

#include "common/str.h"

namespace Amber {

enum GameType {
	kGameTypeAmbermoon,
	kGameTypeAmberstar
};

class GameContext {
public:
	virtual ~GameContext() {}

	virtual GameType getGameType() const = 0;

	// UI Layout Coordinates
	virtual int getMapViewportX() const = 0;
	virtual int getMapViewportY() const = 0;
	virtual int getMapViewportWidth() const = 0;
	virtual int getMapViewportHeight() const = 0;

	// Executable / Main Data file names
	virtual Common::String getMainDataFilename() const = 0;
};

class AmbermoonContext : public GameContext {
public:
	GameType getGameType() const override { return kGameTypeAmbermoon; }

	int getMapViewportX() const override { return 16; }
	int getMapViewportY() const override { return 49; }
	int getMapViewportWidth() const override { return 176; }
	int getMapViewportHeight() const override { return 144; }

	Common::String getMainDataFilename() const override { return "AM2_CPU"; }
};

class AmberstarContext : public GameContext {
public:
	GameType getGameType() const override { return kGameTypeAmberstar; }

	// TODO: Update these once we measure Amberstar's UI layout
	int getMapViewportX() const override { return 16; }
	int getMapViewportY() const override { return 49; }
	int getMapViewportWidth() const override { return 176; }
	int getMapViewportHeight() const override { return 144; }

	Common::String getMainDataFilename() const override { return "AMBERDEV.UDO"; }
};

} // End of namespace Amber
#endif // AMBER_GAME_CONTEXT_H
