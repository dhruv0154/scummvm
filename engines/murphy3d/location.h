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
#include "murphy3d/enums.h"

namespace Murphy3d {

#define MAX_LOCATION_ANIMATIONS 100

struct Box {
	float x1, y1, z1;
	float x2, y2, z2;
};

struct PolygonPoint {
	Math::Vector3d pos;
	float u, v;
};

struct LocationTriangle {
	PolygonPoint p1;
	PolygonPoint p2;
	PolygonPoint p3;
};

struct SpriteInfo {
	int32 textureIndex;
	Math::Vector3d pos;
	float ox, oy;
	float w, h;
	float u1, v1, u2, v2;
	int objectIndex;
	int subObjectIndex;
	uint32 subObjectId;
};

struct SpritePosInfo {
	Math::Vector3d position;
	bool visible;
};

struct LocationSubObject {
	uint32 id;
	int32 subObjectIndex;
	int32 textureIndex;
	int32 originalTextureIndex;
	uint32 flags;
	uint32 pointCount;
	bool visible;

	Box boundingBox;
	Common::Array<LocationTriangle> triangles;
	Common::Array<SpriteInfo> sprites;
};

struct LocationObject {
	uint32 flags;
	bool visible;
	Math::Vector3d translation;
	Box boundingBox;
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
	Texture *overrideTexture;
	Common::Array<Texture *> animatedTextures;
	int32 animatedTextureIndex;
	bool transparent;
	bool rotated;

	uint32 vbo;
	uint32 vertexCount;
	Common::Array<TEXTURED_VERTEX> vertices;

	uint32 spriteVbo;
	uint32 spriteVertexCount;
	Common::Array<TEXTURED_VERTEX> spriteVertices;
};

struct LocationAnimation {
	int32 type;
	AnimationStatus status;
	uint32 frameDuration;
	uint32 constantFrameDuration;
	uint32 frameTime;
	uint32 animDataOffset;
	uint32 animDataOffsetInit;
	uint32 animDataOffsetEnd;
	int32 parentAnim;
	int32 objectId;
	int32 parameter;
	int32 frameCounter;
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

	void startMappedAnimation(int index);
	void startIndexedAnimation(int index);
	void startIdAnimation(int index);
	void stopMappedAnimation(int index);
	void stopIndexedAnimation(int index);
	bool isAnimationFinished(int index) const;
	bool isIndexedAnimationFinished(int index) const;
	int getAnimationFrame(int index) const;
	int getIndexedAnimationFrame(int index) const;

	bool _pointingChanged;
	static bool _disableClipping;

	int getPickObject(int &objectId, int &subObjectId);
	void setObjectVisibility(int objectId, bool visible);
	void changeVisibility(int id, bool visible, bool setOnSubObjects = false);

	double getPlayerDistanceFromPoint(double x, double z);
	Math::Vector3d getPlayerPosition();
	Math::Vector3d getUnadjustedPlayerPosition();
	SpritePosInfo getSpriteInfo(int index);

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
	int _hitObject;
	int _hitSubObject;
	int _objectIndex;

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
	bool intersect(const Box &boundingBox, const Math::Vector3d &from, const Math::Vector3d &invDir);

	void clear();

	static bool _loading;
	static Elevation *_currentElevation;

private:
	Common::String _currentFileName;
	byte _palette[768];
	byte *_animData;
	uint32 _animDataSize;
	int32 _locationAnimationCount;
	LocationAnimation _animations[MAX_LOCATION_ANIMATIONS];

	Common::Array<Math::Vector3d> _vertices;
	Common::Array<LocationObject> _objects;
	Common::Array<Texture *> _textures;
	Common::Array<TextureGroup> _textureGroups;
	Common::Array<Path> _paths;
	Common::Array<Elevation> _elevations;

	Common::HashMap<int, bool> _transparentTextures;
	Common::HashMap<int, bool> _processedTextures;

	uint32 _transparentVbo;
	uint32 _transparentVertexCount;
	Common::Array<COLOURED_VERTEX> _transparentVertices;

	bool _buffersDirty;
	bool _spritesDirty;

	bool loadAnimations(Archive &archive);
	bool loadTextures(Archive &archive);
	bool loadPalette(Archive &archive);
	bool loadGeometry(Archive &archive);
	bool loadPaths(Archive &archive);

	void modifyLocationPoints(const Common::String &file, uint32 objectCount);
	void modifyLocationPoints(uint32 objectCount, int startix, int endix, float x, float y, float z);
	Math::Vector4d getTransparentColour(const Common::String &file, int objectId, int subObjectId) const;

	void rebuildGeometryVertices();
};

} // End of namespace Murphy3d

#endif
