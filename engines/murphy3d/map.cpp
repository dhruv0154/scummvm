#include "murphy3d/map.h"
#include "common/endian.h"
#include "math.h"

namespace Murphy3d {

Map::~Map() {
	for (uint i = 0; i < _entries.size(); i++) {
		delete _entries[i];
	}
	_entries.clear();
}

MapData *Map::get(int entry) {
	if (entry >= 0 && (uint)entry < _entries.size()) {
		return _entries[entry];
	}
	return nullptr;
}

StartupPosition Map::getStartupPosition(int index, int entry) {
	StartupPosition sp;
	memset(&sp, 0, sizeof(StartupPosition));

	if (index >= 0 && (uint)index < _entries.size()) {
		MapData *pMap = _entries[index];
		if (pMap != nullptr && entry >= 0 && (uint)entry < pMap->startupPositions.size()) {
			sp = pMap->startupPositions[entry];
		}
	}

	return sp;
}

uint32 Map::readStartupPositions(MapData *pMapdata, const byte *data, uint32 offset, int numberOfStartupPositions, int positionDataStructSize) {
	if (pMapdata == nullptr || data == nullptr)
		return offset;

	for (int p = 0; p < numberOfStartupPositions; p++) {
		// the structure in the map file is 10 bytes long per entry
		int16 ix = READ_LE_INT16(data + offset);
		int16 iz = READ_LE_INT16(data + offset + 2);
		int16 iy = READ_LE_INT16(data + offset + 4);
		int16 iym = READ_LE_INT16(data + offset + 6);
		int16 a = READ_LE_INT16(data + offset + 8);

		float x = (float)ix / 16.0f;
		float z = (float)iz / 16.0f;
		float y = (float)iy / 16.0f;
		float ym = (float)iym / 16.0f;
		float dy = (y - ym) / 6.0f;
		float fa = (float)a / 10.0f;

		StartupPosition pos;
		pos.x = x;
		pos.y = y;
		pos.z = z;
		pos.initialEyeLevel = -dy * 6.0f;
		pos.minYAdj = -dy * 8.0f;
		pos.maxYAdj = -dy * 1.5f;
		pos.elevation = -ym;
		pos.angle = -fa * 3.141592654f / 180.0f;

		pMapdata->startupPositions.push_back(pos);

		offset += positionDataStructSize;
	}

	return offset;
}

} // End of namespace Murphy3d
