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

#ifndef AMBER_ASSET_LOADER_H
#define AMBER_ASSET_LOADER_H

#include "amber/amiga.h"
#include "graphics/surface.h"

namespace Amber {

class AmberEngine;

class AssetLoader {
public:
	virtual ~AssetLoader() {}

	// virtual methods for extracting core assets
	virtual bool loadCursor(AmberEngine *engine) = 0;
	virtual bool loadFont(AmberEngine *engine) = 0;
	virtual bool loadUI(AmberEngine *engine) = 0;
	virtual bool loadButtons(AmberEngine *engine) = 0;
};

class AmbermoonAssetLoader : public AssetLoader {
private:
	AmigaExecutable _exe;
	bool _exeLoaded = false;

	// caches the AM2_CPU parsing so we don't open it multiple times
	bool ensureExeLoaded();

	// takes the 5 byte 1 bit planar array and creates a 6x6 pixel surface
	Graphics::Surface *decodeGlyph(byte *glyphData);

public:
	bool loadCursor(AmberEngine *engine) override;
	bool loadFont(AmberEngine *engine) override;
	bool loadUI(AmberEngine *engine) override;
	bool loadButtons(AmberEngine *engine) override;
};

class AmberstarAssetLoader : public AssetLoader {
private:
	// caching for AMBERDEV.UDO so we only decompress it once
	byte *_devData = nullptr;
	uint32 _devDataSize = 0;
	bool _devLoaded = false;

	bool ensureDevDataLoaded();

	// helpers for amberstar's specific palette formats
	void decodeCompactPalette(Common::SeekableReadStream *stream, byte *paletteOut);
	void decodeWidePalette(Common::SeekableReadStream *stream, byte *paletteOut);

	// helper to load the hardcoded UI palette
	void loadUIPalette(AmberEngine *engine);

	// helper to decode 8x5 1-bit planar fonts
	Graphics::Surface *decodeAmberstarGlyph(byte *glyphData);

public:
	~AmberstarAssetLoader() override;
	bool loadCursor(AmberEngine *engine) override;
	bool loadFont(AmberEngine *engine) override;
	bool loadUI(AmberEngine *engine) override;
	bool loadButtons(AmberEngine *engine) override;
};

} // End of namespace Amber
#endif // AMBER_ASSET_LOADER_H
