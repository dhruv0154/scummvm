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

	Common::Array<bool> objVisible(objectCount);
	Common::Array<Math::Vector3d> objTranslation(objectCount);

	for (uint32 i = 0; i < objectCount; i++) {
		uint32 objectOffset = READ_LE_UINT32(p3d2 + 0x30 + (i * 4)) + 0x30;
		uint32 flags = READ_LE_UINT32(p3d2 + objectOffset);
		objVisible[i] = ((flags & 0x80000000) == 0);
		objTranslation[i] = Math::Vector3d(0.0f, 0.0f, 0.0f);
	}

	Common::SeekableReadStream *animStream = archive.getStream(0);
	if (animStream) {
		uint32 animSize = animStream->size();
		byte *animData = new byte[animSize];
		animStream->read(animData, animSize);

		uint32 animCount = READ_LE_UINT32(animData + 0);
		for (uint32 a = 0; a < animCount; a++) {
			uint32 offset = READ_LE_UINT32(animData + 8 + a * 8);
			uint32 objId = READ_LE_UINT32(animData + 8 + a * 8 + 4);

			if (offset + 12 <= animSize) {
				uint32 type = READ_LE_UINT32(animData + offset);
				uint32 trigger = READ_LE_UINT32(animData + offset + 8);

				if (trigger == 1) {
					uint32 pA = offset + 12;

					if (type == 2 && pA + 12 <= animSize) {
						int32 p1 = READ_LE_INT32(animData + pA);
						int32 p2 = READ_LE_INT32(animData + pA + 4);
						int32 p3 = READ_LE_INT32(animData + pA + 8);
						if (objId < objectCount) {
							objTranslation[objId].x() += (p1 / 65536.0f);
							objTranslation[objId].y() += (p2 / 65536.0f);
							objTranslation[objId].z() += (p3 / 65536.0f);
						}
					} else if (type == 3 && pA + 4 <= animSize) {
						int32 objToShow = READ_LE_INT32(animData + pA);
						if (objId < objectCount)
							objVisible[objId] = false;
						if (objToShow >= 0 && objToShow < (int32)objectCount)
							objVisible[objToShow] = true;
					}
				}
			}
		}
		delete[] animData;
		delete animStream;
	}

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

			if (!objVisible[i] || (subObj.flags & 0x80000000) != 0)
				continue;

			if (points == 1 && (subObj.flags & 4)) {
				// sprite logic
				uint32 subSprites = READ_LE_UINT32(p3d2 + thisSubOffset + 0x38);
				if (subSprites > 0) {
					uint32 pointIndex = READ_LE_UINT32(p3d2 + thisSubOffset + 0x44 + (subSprites * 32)) >> 4;
					Math::Vector3d center = _vertices[pointIndex + objectCount] + objTranslation[i];

					float sww = READ_LE_INT32(p3d2 + thisSubOffset + 0x28) / 65536.0f;
					float swh = READ_LE_INT32(p3d2 + thisSubOffset + 0x2C) / 65536.0f;
					float ssw = READ_LE_INT32(p3d2 + thisSubOffset + 0x30) / 65536.0f;
					float ssh = READ_LE_INT32(p3d2 + thisSubOffset + 0x34) / 65536.0f;

					float tw = 1.0f, th = 1.0f;
					if (subObj.textureIndex >= 0 && (uint32)subObj.textureIndex < _textures.size() && _textures[subObj.textureIndex]) {
						tw = (float)_textures[subObj.textureIndex]->getWidth();
						th = (float)_textures[subObj.textureIndex]->getHeight();
					}

					for (uint32 ss = 0; ss < subSprites; ss++) {
						float offsetX = READ_LE_INT32(p3d2 + thisSubOffset + 0x44 + ss * 32) / 65536.0f;
						float offsetY = READ_LE_INT32(p3d2 + thisSubOffset + 0x48 + ss * 32) / 65536.0f;
						float texX1 = READ_LE_INT32(p3d2 + thisSubOffset + 0x54 + ss * 32) / 65536.0f;
						float texY1 = READ_LE_INT32(p3d2 + thisSubOffset + 0x58 + ss * 32) / 65536.0f;
						float texX2 = 1.0f + (READ_LE_INT32(p3d2 + thisSubOffset + 0x5C + ss * 32) / 65536.0f);
						float texY2 = 1.0f + (READ_LE_INT32(p3d2 + thisSubOffset + 0x60 + ss * 32) / 65536.0f);

						float OX = (sww * offsetX) / ssw;
						float OY = (swh * offsetY) / ssh;
						float W = (sww * (texX2 - texX1)) / ssw;
						float H = (swh * (texY2 - texY1)) / ssh;

						// build a static 2D quad at the object's 3D coordinates
						float sy1 = center.y() - H - OY;
						float sy2 = center.y() - OY;
						float x1 = center.x() - OX - W;
						float x2 = center.x() - OX;
						float z = center.z();

						PolygonPoint p0 = {Math::Vector3d(x1, sy1, z), texX2 / tw, texY2 / th}; // Bottom Left
						PolygonPoint p1 = {Math::Vector3d(x2, sy1, z), texX1 / tw, texY2 / th}; // Bottom Right
						PolygonPoint p2 = {Math::Vector3d(x2, sy2, z), texX1 / tw, texY1 / th}; // Top Right
						PolygonPoint p3 = {Math::Vector3d(x1, sy2, z), texX2 / tw, texY1 / th}; // Top Left

						addTriangleToGroup(subObj.textureIndex, i, j, subObj.flags, p0, p2, p1);
						addTriangleToGroup(subObj.textureIndex, i, j, subObj.flags, p0, p3, p2);
					}
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

					polyPoints[p].pos = _vertices[pix + objectCount] + objTranslation[i];

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

				int startp = 0;
				int pointsLeft = points;
				int failsafe = pointsLeft * 3;

				while (pointsLeft > 2 && failsafe > 0) {
					int prev1 = startp - 1;
					if (prev1 < 0)
						prev1 += pointsLeft;
					int prev2 = startp - 2;
					if (prev2 < 0)
						prev2 += pointsLeft;

					PolygonPoint p0 = polyPoints[startp];
					PolygonPoint p1 = polyPoints[prev1];
					PolygonPoint p2 = polyPoints[prev2];

					Math::Vector3d v1 = p0.pos - p1.pos;
					Math::Vector3d v2 = p1.pos - p2.pos;

					float len1 = v1.length();
					float len2 = v2.length();
					if (len1 > 0.0f)
						v1.normalize();
					if (len2 > 0.0f)
						v2.normalize();

					if (fabs(v1.x() - v2.x()) > 0.001f ||
						fabs(v1.y() - v2.y()) > 0.001f ||
						fabs(v1.z() - v2.z()) > 0.001f) {

						addTriangleToGroup(subObj.textureIndex, i, j, subObj.flags, p0, p2, p1);
						polyPoints.remove_at(prev1);
						pointsLeft--;
						failsafe = pointsLeft * 3;
					} else {
						failsafe--;
					}

					startp++;
					if (startp >= pointsLeft && pointsLeft > 0)
						startp -= pointsLeft;
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
		// Unlike the VGA palettes used by the 2D resources, location
		// palettes already contain full-range 8-bit RGB components.
		palBytes[i * 4 + 0] = _palette[i * 3 + 0];
		palBytes[i * 4 + 1] = _palette[i * 3 + 1];
		palBytes[i * 4 + 2] = _palette[i * 3 + 2];
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
			bool isRotated = (type == 1);
			byte *scanData = tex + scanOffset;
			_textures[t] = new Texture(w, h, scanData, rgbaPalette, true, isRotated);

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
