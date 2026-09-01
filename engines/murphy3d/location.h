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

	bool load(const Common::String &filename);

	const byte *getPalette() const { return _palette; }
	const Common::Array<Math::Vector3d> &getVertices() const { return _vertices; }
	const Common::Array<LocationObject> &getObjects() const { return _objects; }
	const Common::Array<Texture *> &getTextures() const { return _textures; }
	const Common::Array<TextureGroup> &getTextureGroups() const { return _textureGroups; }
	void buildBuffers(Renderer *renderer);
	void render(Renderer *renderer);

private:
	byte _palette[768];
	Common::Array<Math::Vector3d> _vertices;
	Common::Array<LocationObject> _objects;
	Common::Array<Texture *> _textures;
	Common::Array<TextureGroup> _textureGroups;
	
	bool loadTextures(Archive &archive);
	bool loadPalette(Archive &archive);
	bool loadGeometry(Archive &archive);

	void addTriangleToGroup(int32 textureIndex, uint32 objIdx, uint32 subObjIdx, uint32 flags,
							const PolygonPoint &p1, const PolygonPoint &p2, const PolygonPoint &p3);
};

} // End of namespace Murphy3d
#endif
