#ifndef MURPHY3D_LOCATION_H
#define MURPHY3D_LOCATION_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/util.h"
#include "common/str.h"
#include "math/vector3d.h"
#include "murphy3d/archive.h"
#include "murphy3d/texture.h"
#include "murphy3d/renderer.h"
#include "murphy3d/shader_structs.h"
#include "murphy3d/elevation.h"
#include "murphy3d/map.h"

namespace Murphy3d {

struct PolygonPoint {
	Math::Vector3d pos;
	float u, v;
};

struct LocationSubObject {
	uint32 id;
	int32 textureIndex;
	uint32 flags;

	// for sprites
	float spriteWidth;
	float spriteHeight;
	Math::Vector3d spriteCenter;
};

struct LocationObject {
	uint32 flags;
	Common::Array<LocationSubObject> subObjects;
};

struct Path {
	uint32 id;
	Common::Array<Math::Vector3d> points;
	bool enabled;
	bool allowLeave;
};

struct TextureGroup {
	int32 textureIndex;
	uint32 vbo;
	uint32 vertexCount;
	Common::Array<TEXTURED_VERTEX> vertices;
};

class Location {
public:
	Location();
	~Location();

	bool load(int locationFileIndex);
	bool load(const Common::String &filename);

	void render();
	void render(Renderer *renderer);
	void buildBuffers(Renderer *renderer);

	static void setPosition(StartupPosition pos);
	static void setPosition(float x, float y, float z, float angle);
	void move(float mx, float my, float mz, float tmx);
	void deltaAngles(float angle1, float angle2);
	void updateSprites();
	void animate();

	bool _pointingChanged;

	int getPickObject(int &objectId, int &subObjectId);
	void setObjectVisibility(int objectId, bool visible);

	double getPlayerDistanceFromPoint(double x, double z);
	Math::Vector3d getPlayerPosition();
	Math::Vector3d getUnadjustedPlayerPosition();

	MapData *_mapEntry;

	static float _x;
	static float _y;
	static float _z;
	static float _angle1;
	static float _angle2;
	static float _y_player_adjustment;
	static float _y_player_adjustment_min;
	static float _y_player_adjustment_max;
	static float _y_elevation;

	const byte *getPalette() const { return _palette; }
	const Common::Array<Math::Vector3d> &getVertices() const { return _vertices; }
	const Common::Array<LocationObject> &getObjects() const { return _objects; }
	const Common::Array<Texture *> &getTextures() const { return _textures; }
	const Common::Array<TextureGroup> &getTextureGroups() const { return _textureGroups; }
	const Common::Array<Path> &getPaths() const { return _paths; }

protected:
	void updateY() {
		_y = _y_elevation + _y_player_adjustment;
	}

	void clear();

	static bool _loading;
	static Elevation *_currentElevation;

private:
	byte _palette[768];
	Common::Array<Math::Vector3d> _vertices;
	Common::Array<LocationObject> _objects;
	Common::Array<Texture *> _textures;
	Common::Array<TextureGroup> _textureGroups;
	Common::Array<Path> _paths;
	Common::Array<Elevation> _elevations;

	bool loadTextures(Archive &archive);
	bool loadPalette(Archive &archive);
	bool loadGeometry(Archive &archive);
	bool loadPaths(Archive &archive);
	void addTriangleToGroup(int32 textureIndex, uint32 flags, const PolygonPoint &p1, const PolygonPoint &p2, const PolygonPoint &p3);
};

} // End of namespace Murphy3d

#endif
