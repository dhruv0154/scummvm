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

#ifndef MURPHY3D_MAP_H
#define MURPHY3D_MAP_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Murphy3d {

struct StartupPosition {
	float x, y, z;
	float initialEyeLevel;
	float minYAdj, maxYAdj;
	float elevation, angle;
};

struct FileMap {
	int file;
	int entry;
};

struct MapData {
	int scriptFileIndex;
	int scriptFileEntry;
	int locationFileIndex;

	Common::Array<FileMap> environmentAudioMap;
	Common::Array<FileMap> audioMap;
	Common::Array<FileMap> videoMap;
	Common::Array<FileMap> imageMap;
	Common::Array<int> animationMap;
	Common::Array<uint32> objectMap;

	Common::Array<StartupPosition> startupPositions;
};

class Map {
public:
	virtual ~Map();
	virtual bool init() = 0;

	MapData *get(int entry);
	StartupPosition getStartupPosition(int index, int entry);

protected:
	Common::Array<MapData *> _entries;

	uint32 readStartupPositions(MapData *pMapdata, const byte *data, uint32 offset, int numberOfStartupPositions, int positionDataStructSize);
};

} // End of namespace Murphy3d
#endif
