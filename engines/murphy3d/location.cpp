#include "murphy3d/location.h"
#include "common/endian.h"
#include "common/debug.h"
#include "murphy3d/sqz.h"

namespace Murphy3d {

Location::Location() {
	memset(_palette, 0, sizeof(_palette));
}

Location::~Location() {
	_vertices.clear();
	_objects.clear();

	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i] != nullptr)
			delete _textures[i];
	}
	_textures.clear();
}

void Location::buildBuffers(Renderer *renderer) {
	if (!renderer)
		return;

	int totalVBOs = 0;
	int totalVertices = 0;
	for (uint i = 0; i < _textureGroups.size(); i++) {
		_textureGroups[i].vbo = 0;
		_textureGroups[i].vertexCount = _textureGroups[i].vertices.size();

		if (_textureGroups[i].vertexCount > 0) {
			_textureGroups[i].vbo = renderer->createVertexBuffer(
				_textureGroups[i].vertices.data(),
				_textureGroups[i].vertexCount * sizeof(TEXTURED_VERTEX),
				false
			);

			totalVBOs++;
			totalVertices += _textureGroups[i].vertexCount;
		}
	}

	debug(0, "Murphy3D Pipeline: Built %d VBOs containing a total of %d vertices.", totalVBOs, totalVertices);

	Std140Vec4 defaultVisibility[4096];
	for (int i = 0; i < 4096; i++) {
		defaultVisibility[i].x = 1.0f;
		defaultVisibility[i].y = 1.0f;
		defaultVisibility[i].z = 0.0f;
		defaultVisibility[i].w = 1.0f;
	}
	renderer->updateVisibilityBuffer(defaultVisibility, sizeof(defaultVisibility));


	Std140Vec4 defaultTranslation[256];
	for (int i = 0; i < 256; i++) {
		defaultTranslation[i].x = 0.0f;
		defaultTranslation[i].y = 0.0f;
		defaultTranslation[i].z = 0.0f;
		defaultTranslation[i].w = 0.0f;
	}
	renderer->updateTranslationBuffer(defaultTranslation, sizeof(defaultTranslation));
}

void Location::render(Renderer *renderer) {
	if (!renderer)
		return;

	int drawCalls = 0;
	int skippedNoVerts = 0;
	int skippedNoTexture = 0;


	for (uint i = 0; i < _textureGroups.size(); i++) {

		if (_textureGroups[i].vertexCount > 0) {
			if (_textures[i] != nullptr) {
				renderer->bindTexture(_textures[i]->getId(), 0);

				renderer->drawTexturedTriangles(_textureGroups[i].vbo, _textureGroups[i].vertexCount, 0);

				drawCalls++;
			} else {
				skippedNoTexture++;
			}
		} else {
			skippedNoVerts++; 
		}
	}

	static int frameCounter = 0;
	if (frameCounter++ % 60 == 0) {
		debug(0, "Murphy3D Render Loop: %d Draw Calls executed. Skipped (No Texture): %d. Skipped (No Vertices): %d.",
			  drawCalls, skippedNoTexture, skippedNoVerts);
	}
}

bool Location::load(const Common::String &filename) {
	Archive archive;

	// location files are AP files with at least 7 entries
	if (!archive.open(Common::Path(filename)))
		return false;

	if (!loadPalette(archive))
		return false;

	if (!loadTextures(archive))
		return false;

	if (!loadGeometry(archive))
		return false;

	return true;
}

void Location::addTriangleToGroup(int32 textureIndex, uint32 objIdx, uint32 subObjIdx, uint32 flags,
								  const PolygonPoint &p1, const PolygonPoint &p2, const PolygonPoint &p3) {
	if (textureIndex < 0 || (uint32)textureIndex >= _textureGroups.size())
		return;

	// shaderParameter is 1.0f if transparent, 0.0f if solid
	float shaderParam = (flags & 0x00000010) ? 1.0f : 0.0f;

	TEXTURED_VERTEX v1 = {p1.pos.x(), p1.pos.y(), p1.pos.z(), p1.u, p1.v, (float)objIdx, (float)subObjIdx, shaderParam, 0, 0, 0};
	TEXTURED_VERTEX v2 = {p2.pos.x(), p2.pos.y(), p2.pos.z(), p2.u, p2.v, (float)objIdx, (float)subObjIdx, shaderParam, 0, 0, 0};
	TEXTURED_VERTEX v3 = {p3.pos.x(), p3.pos.y(), p3.pos.z(), p3.u, p3.v, (float)objIdx, (float)subObjIdx, shaderParam, 0, 0, 0};

	_textureGroups[textureIndex].textureIndex = textureIndex;
	_textureGroups[textureIndex].vertices.push_back(v1);
	_textureGroups[textureIndex].vertices.push_back(v2);
	_textureGroups[textureIndex].vertices.push_back(v3);
}

bool Location::loadPalette(Archive &archive) {
	// the third entry is the 8-bit (256 colour) palette used for the location
	Common::SeekableReadStream *stream = archive.getStream(2);
	if (!stream)
		return false;

	uint16 byteCount = stream->readUint16LE();

	// if the stream size exceeds the standard 0x300 (768) bytes
	if (stream->size() >= 770)
		stream->seek(stream->size() - 768);

	// 3 * 256 bytes
	stream->read(_palette, 768);

	delete stream;
	return true;
}

bool Location::loadGeometry(Archive &archive) {
	// the fifth entry defines the 3D location data
	Common::SeekableReadStream *stream = archive.getStream(4);
	if (!stream)
		return false;

	uint32 streamSize = stream->size();
	byte *p3d2 = new byte[streamSize];
	stream->read(p3d2, streamSize);
	delete stream;

	uint32 verticeCount = READ_LE_UINT32(p3d2 + 0x00);
	uint32 objectCount = READ_LE_UINT32(p3d2 + 0x0C);

	// 3D points referenced by objects begin after the Object offset table
	// the offset table is 0x30 + (4 * number of objects)
	uint32 moff = 0x30 + (objectCount * 4);

	_vertices.resize(verticeCount);
	for (uint32 i = 0; i < verticeCount; i++) {
		// coordinates are 16.16 fixed point values
		float x = READ_LE_INT32(p3d2 + moff) / 65536.0f;
		float y = READ_LE_INT32(p3d2 + moff + 4) / 65536.0f;
		float z = READ_LE_INT32(p3d2 + moff + 8) / 65536.0f;

		_vertices[i] = Math::Vector3d(x, y, z);
		moff += 12;
	}

	_objects.resize(objectCount);
	for (uint32 i = 0; i < objectCount; i++) {
		// object offset table, add 0x30 to get real offset in entry
		uint32 objectOffset = READ_LE_UINT32(p3d2 + 0x30 + (i * 4)) + 0x30;

		// object flags (0x80000000 = object is hidden)
		_objects[i].flags = READ_LE_UINT32(p3d2 + objectOffset);

		// number of sub-objects
		uint32 subObjectsCount = READ_LE_UINT32(p3d2 + objectOffset + 12);
		_objects[i].subObjects.resize(subObjectsCount);

		uint32 nextSubOffset = objectOffset + 40;
		for (uint32 j = 0; j < subObjectsCount; j++) {
			uint32 thisSubOffset = nextSubOffset;
			// pointer to next sub-object, add 0x30
			nextSubOffset = READ_LE_UINT32(p3d2 + nextSubOffset) + 0x30;

			LocationSubObject &subObj = _objects[i].subObjects[j];
			uint32 points = READ_LE_UINT32(p3d2 + thisSubOffset + 4);

			// sub-object flags (e.g., 0x00000002 = textured, 0x00000004 = sprite)
			subObj.flags = READ_LE_UINT32(p3d2 + thisSubOffset + 8);
			subObj.id = READ_LE_UINT32(p3d2 + thisSubOffset + 0x0C);
			subObj.textureIndex = (int32)READ_LE_UINT32(p3d2 + thisSubOffset + 0x24);

			if (points == 1 && (subObj.flags & 4)) {
				// sprite logic
				uint32 subSprites = READ_LE_UINT32(p3d2 + thisSubOffset + 0x38);
				if (subSprites > 0) {
					// shift point indexes 4 bits to the
					// right and add object count to get real indexes
					uint32 pointIndex = READ_LE_UINT32(p3d2 +
						thisSubOffset + 0x44 + (subSprites * 32)) >> 4;
					subObj.spriteCenter = _vertices[pointIndex + objectCount];

					// sprite width/height in 3D world units (16.16 fixed point)
					subObj.spriteWidth = READ_LE_INT32(p3d2 +
						thisSubOffset + 0x28) / 65536.0f;
					subObj.spriteHeight = READ_LE_INT32(p3d2 +
						thisSubOffset + 0x2C) / 65536.0f;
				}
			} else if (points > 2 && (subObj.flags & 2)) {

				float tw = 1.0f, th = 1.0f;
				if (subObj.textureIndex >= 0 && (uint32)subObj.textureIndex < _textures.size() && _textures[subObj.textureIndex]) {
					tw = (float)_textures[subObj.textureIndex]->getWidth();
					th = (float)_textures[subObj.textureIndex]->getHeight();
				}

				uint32 uvOffset = thisSubOffset + 0x28 + (points * 4);
				Common::Array<PolygonPoint> polyPoints(points);

				for (uint32 p = 0; p < points; p++) {
					// shift point indexes 4 bits to the
					// right and add object count to get real indexes
					uint32 pix = READ_LE_UINT32(p3d2 +
						thisSubOffset + 0x28 + (p * 4)) >> 4;

					// texture offsets are 16.16 fixed point
					float fu = READ_LE_INT32(p3d2 + uvOffset + (p * 8)) / 65536.0f;
					float fv = READ_LE_INT32(p3d2 + uvOffset + 4 + (p * 8)) / 65536.0f;

					polyPoints[p].pos = _vertices[pix + objectCount];

					// divide by texture width/height to get U/V
					// 0x00000800 indicates single colour mapping
					if (subObj.flags & 0x00000800) {
						polyPoints[p].u = 0.0f;
						polyPoints[p].v = 0.0f;
					} else {
						polyPoints[p].u = CLIP(fu / tw, 0.0f, 1.0f);
						polyPoints[p].v = CLIP(fv / th, 0.0f, 1.0f);
					}
				}

				for (uint32 p = 2; p < points; p++) {
					PolygonPoint p0 = polyPoints[0];
					PolygonPoint p1 = polyPoints[p - 1];
					PolygonPoint p2 = polyPoints[p];
					addTriangleToGroup(subObj.textureIndex, i, j, subObj.flags, p0, p2, p1);
				}
			}
		}
	}

	delete[] p3d2;
	return true;
}

bool Location::loadTextures(Archive &archive) {
	// the sixth entry holds the textures in a compressed .SQZ format
	Common::SeekableReadStream *stream = archive.getStream(5);
	if (!stream)
		return false;

	uint32 streamSize = stream->size();
	byte *compressedData = new byte[streamSize];
	stream->read(compressedData, streamSize);
	delete stream;

	// decompress the .SQZ file
	BinaryData texData = SQZ::decompress(compressedData, streamSize);
	delete[] compressedData;

	if (texData.data == nullptr || texData.length == 0)
		return false;

	byte *tex = texData.data;

	// offset 0x08: number of textures
	uint32 texCount = READ_LE_UINT32(tex + 8);

	// the texture information table sits at offset 0x0C and is 12 bytes per texture
	// the actual texture data begins immediately after this table
	uint32 texPtr = 12 + (texCount * 12);

	// convert our 24-bit RGB palette into a 32-bit RGBA palette array
	// so the texture class can map the 8-bit image data correctly
	uint32 rgbaPalette[256];
	byte *palBytes = (byte *)rgbaPalette;
	for (int i = 0; i < 256; i++) {
		palBytes[i * 4 + 0] = (_palette[i * 3 + 0] * 255) / 63;
		palBytes[i * 4 + 1] = (_palette[i * 3 + 1] * 255) / 63;
		palBytes[i * 4 + 2] = (_palette[i * 3 + 2] * 255) / 63;
		palBytes[i * 4 + 3] = 255;
	}

	_textures.resize(texCount);

	for (uint32 t = 0; t < texCount; t++) {
		// read width, height and type
		uint32 w = READ_LE_UINT32(tex + texPtr);
		uint32 h = READ_LE_UINT32(tex + texPtr + 4);
		uint32 type = READ_LE_UINT32(tex + texPtr + 8);

		texPtr += 12;

		// type 0 = normal, type 1 = rotated
		if (type == 0 || type == 1) {
			// skip 4 * height bytes
			uint32 scanOffset = texPtr + (4 * h);

			// the next width * height bytes make up the texture
			byte *scanData = tex + scanOffset;
			_textures[t] = new Texture(w, h, scanData, rgbaPalette, true);

			texPtr = scanOffset + (w * h);

		} else if (type == 2) {
			// we will skip rendering animations for now
			texPtr += w;
			_textures[t] = nullptr;
		} else {
			_textures[t] = nullptr;
		}
	}
	_textureGroups.resize(_textures.size());
	delete[] texData.data;

	int validTextures = 0;
	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i] != nullptr)
			validTextures++;
	}
	debug(0, "Murphy3D Pipeline: Loaded %d valid textures out of %d total.", validTextures, texCount);
	return true;
}

} // End of namespace Murphy3d
