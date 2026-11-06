#include "common/endian.h"
#include "common/debug.h"
#include "murphy3d/location.h"
#include "common/system.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/sqz.h"
#include "murphy3d/math_utils.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/game_base.h"
#include "murphy3d/uakm_game.h"

namespace Murphy3d {

static const float kTwoPi = 6.283185307f;
static const float kHalfPi = 3.141592654f / 2.0f;

#define OBJECT_FLAGS_HIDDEN 0x80000000
#define SUBOBJECT_FLAGS_TRANSPARENT 0x00000001
#define SUBOBJECT_FLAGS_TEXTURED 0x00000002
#define SUBOBJECT_FLAGS_SPRITE 0x00000004
#define SUBOBJECT_FLAGS_ALPHA 0x00000010
#define SUBOBJECT_FLAGS_SINGLE_COLOUR 0x00000800
#define SUBOBJECT_FLAGS_HIDDEN 0x80000000
#define TEXTURE_FLAGS_LARGE 0x00000200

bool Location::_loading = false;
bool Location::_disableClipping = false;
float Location::_x = 0.0f;
float Location::_y = 0.0f;
float Location::_z = 0.0f;
float Location::_angle1 = 0.0f;
float Location::_angle2 = 0.0f;
float Location::_y_player_adjustment = 0.0f;
float Location::_y_player_adjustment_min = 0.0f;
float Location::_y_player_adjustment_max = 0.0f;
float Location::_y_elevation = 0.0f;
Elevation *Location::_currentElevation = nullptr;

Location::Location() {
	memset(_palette, 0, sizeof(_palette));
	memset(_animations, 0, sizeof(_animations));
	_animData = nullptr;
	_animDataSize = 0;
	_locationAnimationCount = 0;

	_loading = false;
	_pointingChanged = false;
	_mapEntry = nullptr;
	_hitObject = -1;
	_hitSubObject = -1;
	_objectIndex = -1;

	_transparentVbo = 0;
	_transparentVertexCount = 0;
	_buffersDirty = false;
	_spritesDirty = false;

	_x = 0.0f;
	_y = 0.0f;
	_z = 0.0f;
	_angle1 = 0.0f;
	_angle2 = 0.0f;
	_y_player_adjustment = -1.5f;
	_y_player_adjustment_min = 0.0f;
	_y_player_adjustment_max = 0.0f;
	_y_elevation = 0.0f;
	_currentElevation = nullptr;
}

Location::~Location() {
	clear();
}

void Location::clear() {
	_vertices.clear();
	_objects.clear();
	_paths.clear();
	_elevations.clear();
	_transparentTextures.clear();
	_processedTextures.clear();
	_transparentVertices.clear();
	_currentElevation = nullptr;

	delete[] _animData;
	_animData = nullptr;
	_animDataSize = 0;
	_locationAnimationCount = 0;
	memset(_animations, 0, sizeof(_animations));

	if (_transparentVbo != 0 && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_transparentVbo);
	}
	_transparentVbo = 0;
	_transparentVertexCount = 0;

	for (uint i = 0; i < _textureGroups.size(); i++) {
		if (_textureGroups[i].vbo != 0 && g_engine && g_engine->_renderer) {
			g_engine->_renderer->deleteBuffer(_textureGroups[i].vbo);
		}
		if (_textureGroups[i].spriteVbo != 0 && g_engine && g_engine->_renderer) {
			g_engine->_renderer->deleteBuffer(_textureGroups[i].spriteVbo);
		}
		for (uint a = 0; a < _textureGroups[i].animatedTextures.size(); a++) {
			delete _textureGroups[i].animatedTextures[a];
		}
		_textureGroups[i].animatedTextures.clear();
	}
	_textureGroups.clear();

	for (uint i = 0; i < _textures.size(); i++) {
		if (_textures[i] != nullptr)
			delete _textures[i];
	}
	_textures.clear();

	_buffersDirty = false;
	_spritesDirty = false;
}

bool Location::load(int locationFileIndex) {
	Common::String file = GameController::getFileName(locationFileIndex);
	if (file.empty()) {
		return false;
	}
	return load(file);
}

bool Location::load(const Common::String &filename) {
	_loading = true;
	_pointingChanged = true;

	clear();
	_currentFileName = filename;

	Archive archive;
	if (!archive.open(Common::Path(filename))) {
		_loading = false;
		return false;
	}

	if (!loadAnimations(archive) || !loadPalette(archive) || !loadPaths(archive) || !loadGeometry(archive)) {
		_loading = false;
		return false;
	}

	// start initial animations (trigger == 1) and build elevation ramps (type == 16)
	if (_animData && _animDataSize >= 4) {
		for (int i = 0; i < _locationAnimationCount && i < MAX_LOCATION_ANIMATIONS; i++) {
			if ((uint32)(8 + i * 8 + 4) > _animDataSize)
				break;
			uint32 offset = READ_LE_UINT32(_animData + 8 + i * 8);
			if (offset + 12 > _animDataSize)
				continue;
			uint32 type = READ_LE_UINT32(_animData + offset);
			uint32 trigger = READ_LE_UINT32(_animData + offset + 8);
			if (trigger == 1) {
				startIndexedAnimation(i);
			} else if (type == 16) {
				Elevation newElevation(_animData + offset);
				_elevations.push_back(newElevation);
			}
		}
	}

	_loading = false;
	animate();

	rebuildGeometryVertices();
	if (g_engine && g_engine->_renderer) {
		buildBuffers(g_engine->_renderer);
	}
	updateSprites();

	return true;
}

bool Location::loadAnimations(Archive &archive) {
	Common::SeekableReadStream *animStream = archive.getStream(0);
	if (!animStream)
		return false;

	_animDataSize = animStream->size();
	if (_animDataSize >= 4) {
		_animData = new byte[_animDataSize];
		animStream->read(_animData, _animDataSize);
		_locationAnimationCount = (int32)READ_LE_UINT32(_animData + 0);
		if (_locationAnimationCount > MAX_LOCATION_ANIMATIONS)
			_locationAnimationCount = MAX_LOCATION_ANIMATIONS;
	}
	delete animStream;
	return true;
}

void Location::rebuildGeometryVertices() {
	for (uint t = 0; t < _textureGroups.size(); t++) {
		_textureGroups[t].vertices.clear();
	}
	_transparentVertices.clear();

	for (uint i = 0; i < _objects.size(); i++) {
		const LocationObject &obj = _objects[i];
		if (!obj.visible)
			continue;

		const Math::Vector3d &trans = obj.translation;

		for (uint j = 0; j < obj.subObjects.size(); j++) {
			const LocationSubObject &subObj = obj.subObjects[j];
			if (!subObj.visible || subObj.pointCount < 3)
				continue;

			if ((subObj.flags & SUBOBJECT_FLAGS_TRANSPARENT) != 0) {
				Math::Vector4d col = getTransparentColour(_currentFileName, (int)i, (int)(subObj.id & 0xffff));
				for (uint k = 0; k < subObj.triangles.size(); k++) {
					const LocationTriangle &tri = subObj.triangles[k];
					Math::Vector3d p1 = tri.p1.pos + trans;
					Math::Vector3d p2 = tri.p2.pos + trans;
					Math::Vector3d p3 = tri.p3.pos + trans;

					_transparentVertices.push_back({p1.x(), p1.y(), p1.z(), col.x(), col.y(), col.z(), col.w()});
					_transparentVertices.push_back({p2.x(), p2.y(), p2.z(), col.x(), col.y(), col.z(), col.w()});
					_transparentVertices.push_back({p3.x(), p3.y(), p3.z(), col.x(), col.y(), col.z(), col.w()});
				}
			} else if ((subObj.flags & SUBOBJECT_FLAGS_TEXTURED) != 0 &&
					   subObj.textureIndex >= 0 && (uint32)subObj.textureIndex < _textureGroups.size()) {
				Common::Array<TEXTURED_VERTEX> &verts = _textureGroups[subObj.textureIndex].vertices;
				for (uint k = 0; k < subObj.triangles.size(); k++) {
					const LocationTriangle &tri = subObj.triangles[k];
					Math::Vector3d p1 = tri.p1.pos + trans;
					Math::Vector3d p2 = tri.p2.pos + trans;
					Math::Vector3d p3 = tri.p3.pos + trans;

					verts.push_back({p1.x(), p1.y(), p1.z(), tri.p1.u, tri.p1.v});
					verts.push_back({p2.x(), p2.y(), p2.z(), tri.p2.u, tri.p2.v});
					verts.push_back({p3.x(), p3.y(), p3.z(), tri.p3.u, tri.p3.v});
				}
			}
		}
	}
}

void Location::buildBuffers(Renderer *renderer) {
	if (!renderer)
		return;

	for (uint i = 0; i < _textureGroups.size(); i++) {
		_textureGroups[i].vertexCount = _textureGroups[i].vertices.size();

		if (_textureGroups[i].vertexCount > 0) {
			if (_textureGroups[i].vbo == 0) {
				_textureGroups[i].vbo = renderer->createVertexBuffer(
					_textureGroups[i].vertices.data(),
					_textureGroups[i].vertexCount * sizeof(TEXTURED_VERTEX),
					true);
			} else {
				renderer->updateBufferData(
					_textureGroups[i].vbo,
					_textureGroups[i].vertices.data(),
					_textureGroups[i].vertexCount * sizeof(TEXTURED_VERTEX),
					0);
			}
		}
	}

	_transparentVertexCount = _transparentVertices.size();
	if (_transparentVertexCount > 0) {
		if (_transparentVbo == 0) {
			_transparentVbo = renderer->createVertexBuffer(
				_transparentVertices.data(),
				_transparentVertexCount * sizeof(COLOURED_VERTEX),
				true);
		} else {
			renderer->updateBufferData(
				_transparentVbo,
				_transparentVertices.data(),
				_transparentVertexCount * sizeof(COLOURED_VERTEX),
				0);
		}
	}

	_buffersDirty = false;
}

void Location::render() {
	if (g_engine && g_engine->_renderer) {
		render(g_engine->_renderer);
	}
}

void Location::render(Renderer *renderer) {
	if (!renderer)
		return;

	if (_buffersDirty) {
		rebuildGeometryVertices();
		buildBuffers(renderer);
		_spritesDirty = true;
	}
	if (_spritesDirty) {
		updateSprites();
	}

	updateY();

	Math::Matrix4 rm1 = MathUtils::rotationX(_angle1);
	Math::Matrix4 rm2 = MathUtils::rotationY(_angle2);
	Math::Matrix4 tm = MathUtils::translation(_x, _y, _z);
	Math::Matrix4 worldMat = tm * rm2 * rm1;

	Math::Matrix4 viewMat = MathUtils::lookAtLH(
		Math::Vector3d(0.0f, 0.0f, 0.0f),
		Math::Vector3d(0.0f, 0.0f, 1.0f),
		Math::Vector3d(0.0f, 1.0f, 0.0f));
	float fov = (3.141592654f / 4.0f) / 0.95f;
	Math::Matrix4 projMat = MathUtils::perspectiveFovLH(fov, 640.0f / 480.0f, 0.1f, 1000.0f);

	renderer->updateMatrices(worldMat, viewMat, projMat);

	// draw textured 3D geometry
	for (uint i = 0; i < _textureGroups.size(); i++) {
		Texture *activeTex = (_textureGroups[i].overrideTexture != nullptr) ? _textureGroups[i].overrideTexture : _textures[i];
		if (_textureGroups[i].vertexCount > 0 && activeTex != nullptr) {
			renderer->bindTexture(activeTex->getId(), 0);
			renderer->drawTexturedTriangles(_textureGroups[i].vbo, _textureGroups[i].vertexCount, 0);
		}
	}

	// draw billboard sprites
	for (uint i = 0; i < _textureGroups.size(); i++) {
		Texture *activeTex = (_textureGroups[i].overrideTexture != nullptr) ? _textureGroups[i].overrideTexture : _textures[i];
		if (_textureGroups[i].spriteVertexCount > 0 && activeTex != nullptr) {
			renderer->bindTexture(activeTex->getId(), 0);
			renderer->drawTexturedTriangles(_textureGroups[i].spriteVbo, _textureGroups[i].spriteVertexCount, 0);
		}
	}

	// draw semi transparent coloured geometry
	if (_transparentVertexCount > 0 && _transparentVbo != 0) {
		renderer->drawColouredTriangles(_transparentVbo, _transparentVertexCount, 0);
	}
}

void Location::setPosition(StartupPosition pos) {
	_x = pos.x;
	_y = pos.y;
	_y_player_adjustment = pos.initialEyeLevel;
	_y_player_adjustment_min = pos.minYAdj;
	_y_player_adjustment_max = pos.maxYAdj;
	_y_elevation = pos.elevation;

	_z = pos.z;
	_angle1 = 0.0f;
	_angle2 = pos.angle;

	_currentElevation = nullptr;
}

void Location::setPosition(float x, float y, float z, float angle) {
	_x = x;
	_y = y;
	_z = z;
	_angle2 = angle;
}

bool Location::loadPaths(Archive &archive) {
	// the collision paths are stored in entry 1 of the location archive
	Common::SeekableReadStream *stream = archive.getStream(1);
	if (!stream)
		return false;

	_paths.clear();

	// header
	stream->seek(0x18);
	uint16 pathVerticeCount = stream->readUint16LE();
	uint16 pathCount = stream->readUint16LE();

	// the vertex data starts at offset 0x80
	stream->seek(0x80);

	// we store the vertices, the paths will reference these by indices
	Common::Array<Math::Vector3d> rawPoints;
	rawPoints.resize(pathVerticeCount);

	for (uint16 v = 0; v < pathVerticeCount; v++) {
		// coordinates are stored as 16.16 fixed point signed integers
		float x = (int32)stream->readUint32LE() / 65536.0f;
		float y = (int32)stream->readUint32LE() / 65536.0f;
		float z = (int32)stream->readUint32LE() / 65536.0f;

		rawPoints[v] = Math::Vector3d(x, y, z);
	}

	// we read the actual paths which are defined as "Faces"
	for (uint16 f = 0; f < pathCount; f++) {
		uint32 chunkStart = stream->pos();
		uint32 tag = stream->readUint32LE();

		// check for the "FACE" tag
		if (tag == 0x45434146) {
			Path newPath;
			newPath.enabled = true;
			newPath.allowLeave = false;

			uint32 faceSize = stream->readUint32LE();
			uint32 vertCount = stream->readUint32LE();

			// path ID is at relative offset 0x14
			stream->seek(chunkStart + 14);
			uint16 pathId = stream->readUint16LE();
			newPath.id = (pathId != 0) ? pathId : f;

			// vertex indices are starting at relative offset 0x15
			stream->seek(chunkStart + 0x15);
			for (uint32 v = 0; v < vertCount; v++) {
				uint16 pointIndex = stream->readUint16LE();
				if (pointIndex < pathVerticeCount) {
					newPath.points.push_back(rawPoints[pointIndex]);
				}
			}

			_paths.push_back(newPath);

			// bypassing padding at the end of the block
			stream->seek(chunkStart + faceSize);
		} else {
			break;
		}
	}

	delete stream;
	return true;
}

void Location::move(float mx, float my, float mz, float tmx) {
	if (mx != 0.0f || my != 0.0f || mz != 0.0f) {
		_pointingChanged = true;
	}

	if (mx != 0.0f || mz != 0.0f) {
		double nx = _x - (mz * sin(_angle2) - mx * cos(_angle2));
		double nz = _z + (mz * cos(_angle2) + mx * sin(_angle2));

		Math::Vector3d pp1(-_x, -_y_elevation, -_z);
		Math::Vector3d pp2((float)-nx, -_y_elevation, (float)-nz);

		bool collision = !_disableClipping;
		for (int pass = 0; pass < 2 && collision; pass++) {
			collision = false;

			double closest_t = 10000.0;
			double closest_u = 0.0;
			double closest_dist = 10000.0;
			Math::Vector3d closestSegmentP1, closestSegmentP2;

			for (uint i = 0; i < _paths.size(); i++) {
				if (!_paths[i].enabled)
					continue;

				for (uint p1_idx = 0; p1_idx < _paths[i].points.size(); p1_idx++) {
					uint p2_idx = (p1_idx + 1) % _paths[i].points.size();
					Math::Vector3d pt1 = _paths[i].points[p1_idx];
					Math::Vector3d pt2 = _paths[i].points[p2_idx];

					double t = 0.0, u = 0.0;
					double d = MathUtils::lineSegmentsDistanceXZ(pp1, pp2, pt1, pt2, t, u);

					if (d < 0.5 && d < closest_dist) {
						closestSegmentP1 = pt1;
						closestSegmentP2 = pt2;
						closest_t = t;
						closest_u = u;
						closest_dist = d;
						collision = true;
					}
				}
			}

			if (collision) {
				Math::Vector3d mp = MathUtils::projectXZ(closestSegmentP1, closestSegmentP2, pp2);
				Math::Vector3d pp = MathUtils::projectXZ(closestSegmentP1, closestSegmentP2, pp1);

				double playerDistanceToLine = sqrt(MathUtils::distanceSquaredXZ(pp1, pp));
				double movementDistanceToLine = sqrt(MathUtils::distanceSquaredXZ(pp2, mp));

				double distanceDelta = playerDistanceToLine - movementDistanceToLine;
				double percentage = 1.0;

				if (playerDistanceToLine > 0.5 && movementDistanceToLine <= 0.5) {
					percentage = (distanceDelta - (0.5001 - movementDistanceToLine)) / distanceDelta;
				} else if (playerDistanceToLine <= 0.5 && movementDistanceToLine >= playerDistanceToLine) {
					percentage = 1.0;
				} else if (playerDistanceToLine < 0.5 && movementDistanceToLine < playerDistanceToLine) {
					nx = _x;
					nz = _z;
					break;
				} else {
					percentage = 0.0;
				}

				Math::Vector3d tp(
					pp1.x() + (pp2.x() - pp1.x()) * (float)percentage,
					0.0f,
					pp1.z() + (pp2.z() - pp1.z()) * (float)percentage);
				Math::Vector3d tpp = MathUtils::projectXZ(closestSegmentP1, closestSegmentP2, tp);
				Math::Vector3d tmp = mp - tpp;

				pp1 = tp;
				pp2 = Math::Vector3d(tp.x() + tmp.x(), 0.0f, tp.z() + tmp.z());

				nx = (pass == 0) ? -pp2.x() : -tp.x();
				nz = (pass == 0) ? -pp2.z() : -tp.z();
			}
		}

		_x = (float)nx;
		_z = (float)nz;

		if (!_disableClipping) {
			Math::Vector3d worldPos(-_x, -_y_elevation, -_z);
			if (_currentElevation != nullptr) {
				if (_currentElevation->isPointInElevation(worldPos)) {
					_y_elevation = -_currentElevation->getElevationFromXZPosition(worldPos);
				} else {
					_y_elevation = -_currentElevation->getClosestElevation(-_y_elevation);
				}
				_currentElevation = nullptr;
			}

			if (_currentElevation == nullptr) {
				for (uint i = 0; i < _elevations.size(); i++) {
					if (_elevations[i].isPointInElevation(worldPos)) {
						_y_elevation = -_elevations[i].getElevationFromXZPosition(worldPos);
						_currentElevation = &_elevations[i];
						break;
					}
				}
			}
		}
	}

	if (my != 0.0f) {
		float miny = _disableClipping ? -1000.0f : _y_player_adjustment_min;
		float maxy = _disableClipping ? 1000.0f : _y_player_adjustment_max;
		if (_y_player_adjustment <= maxy && _y_player_adjustment >= miny) {
			_y_player_adjustment += (my < 0.0f) ? MAX(my, miny - _y_player_adjustment) : MIN(my, maxy - _y_player_adjustment);
		}
	}

	updateSprites();
}

void Location::deltaAngles(float angle1, float angle2) {
	if (angle1 != 0.0f || angle2 != 0.0f) {
		_pointingChanged = true;
	}

	_angle1 -= angle1;
	_angle2 -= angle2;

	while (_angle2 < 0.0f) {
		_angle2 += kTwoPi;
	}
	while (_angle2 > kTwoPi) {
		_angle2 -= kTwoPi;
	}

	if (_angle1 > kHalfPi)
		_angle1 = kHalfPi;
	else if (_angle1 < -kHalfPi)
		_angle1 = -kHalfPi;
}

void Location::updateSprites() {
	if (!g_engine || !g_engine->_renderer)
		return;

	for (uint t = 0; t < _textureGroups.size(); t++) {
		_textureGroups[t].spriteVertices.clear();
	}

	for (uint i = 0; i < _objects.size(); i++) {
		if (!_objects[i].visible)
			continue;

		const Math::Vector3d &trans = _objects[i].translation;

		for (uint j = 0; j < _objects[i].subObjects.size(); j++) {
			const LocationSubObject &subObj = _objects[i].subObjects[j];
			if (!subObj.visible || subObj.sprites.empty())
				continue;

			int32 texIdx = subObj.textureIndex;
			if (texIdx < 0 || (uint32)texIdx >= _textureGroups.size())
				continue;

			Common::Array<TEXTURED_VERTEX> &sVerts = _textureGroups[texIdx].spriteVertices;

			for (uint s = 0; s < subObj.sprites.size(); s++) {
				const SpriteInfo &spr = subObj.sprites[s];
				Math::Vector3d center = spr.pos + trans;

				float svx = -_x - center.x();
				float svz = -_z - center.z();
				float len = sqrt(svx * svx + svz * svz);
				if (len == 0.0f)
					len = 1.0f;
				svx /= len;
				svz /= len;

				float sy1 = center.y() - spr.h - spr.oy;
				float sy2 = center.y() - spr.oy;

				float x1 = center.x() - (spr.ox + spr.w) * svz;
				float x2 = center.x() - spr.ox * svz;
				float z1 = center.z() + (spr.ox + spr.w) * svx;
				float z2 = center.z() + spr.ox * svx;

				sVerts.push_back({x1, sy1, z1, spr.u2, spr.v2});
				sVerts.push_back({x2, sy1, z2, spr.u1, spr.v2});
				sVerts.push_back({x2, sy2, z2, spr.u1, spr.v1});

				sVerts.push_back({x1, sy1, z1, spr.u2, spr.v2});
				sVerts.push_back({x2, sy2, z2, spr.u1, spr.v1});
				sVerts.push_back({x1, sy2, z1, spr.u2, spr.v1});
			}
		}
	}

	Renderer *renderer = g_engine->_renderer;
	for (uint t = 0; t < _textureGroups.size(); t++) {
		_textureGroups[t].spriteVertexCount = _textureGroups[t].spriteVertices.size();
		if (_textureGroups[t].spriteVertexCount > 0) {
			if (_textureGroups[t].spriteVbo == 0) {
				_textureGroups[t].spriteVbo = renderer->createVertexBuffer(
					_textureGroups[t].spriteVertices.data(),
					_textureGroups[t].spriteVertexCount * sizeof(TEXTURED_VERTEX),
					true);
			} else {
				renderer->updateBufferData(
					_textureGroups[t].spriteVbo,
					_textureGroups[t].spriteVertices.data(),
					_textureGroups[t].spriteVertexCount * sizeof(TEXTURED_VERTEX),
					0);
			}
		}
	}

	_spritesDirty = false;
}

void Location::startMappedAnimation(int index) {
	if (_mapEntry && index >= 0 && (uint)index < _mapEntry->animationMap.size()) {
		startIndexedAnimation(_mapEntry->animationMap[index]);
	}
}

void Location::startIndexedAnimation(int index) {
	if (!_animData || index < 0 || index >= _locationAnimationCount || index >= MAX_LOCATION_ANIMATIONS)
		return;

	if (_animations[index].status == AnimationStatus::kNotStarted || _animations[index].status == AnimationStatus::kCompleted) {
		if ((uint32)(index * 8 + 12) > _animDataSize)
			return;

		_animations[index].objectId = READ_LE_INT32(_animData + index * 8 + 4);
		uint32 pA = READ_LE_UINT32(_animData + index * 8 + 8);
		uint32 pAE = (index < (_locationAnimationCount - 1)) ? READ_LE_UINT32(_animData + index * 8 + 16) : _animDataSize;

		if (pA + 12 > _animDataSize)
			return;

		int32 id = READ_LE_INT32(_animData + pA + 4);
		if (id == index) {
			_animations[index].type = READ_LE_INT32(_animData + pA);
			pA += 12;

			_animations[index].animDataOffsetInit = pA;
			_animations[index].animDataOffsetEnd = pAE;
			_animations[index].constantFrameDuration = 0;

			if (_animations[index].type == 1 && pA + 4 <= _animDataSize) {
				_animations[index].parameter = READ_LE_INT32(_animData + pA);
				pA += 4;
			} else if (_animations[index].type == 15 && pA + 8 <= _animDataSize) {
				_animations[index].parameter = READ_LE_INT32(_animData + pA);
				_animations[index].frameDuration = _animations[index].constantFrameDuration = (uint32)(READ_LE_INT32(_animData + pA + 4) * TIMER_SCALE);
				pA += 8;
				_animations[index].animDataOffsetInit = pA;
			} else {
				_animations[index].parameter = (_animations[index].type == 3) ? _animations[index].objectId : -1;
			}

			_animations[index].animDataOffset = pA;
			_animations[index].frameCounter = 0;
			_animations[index].frameDuration = 0;
			_animations[index].frameTime = 0;
			_animations[index].parentAnim = -1;
			_animations[index].status = AnimationStatus::kRunning;
		}
	}
}

void Location::startIdAnimation(int index) {
	if (!_mapEntry || !_animData || index < 0 || (uint)index >= _mapEntry->animationMap.size())
		return;

	int mappedId = _mapEntry->animationMap[index];
	for (int i = 0; i < _locationAnimationCount && i < MAX_LOCATION_ANIMATIONS; i++) {
		int animId = READ_LE_INT32(_animData + i * 8 + 4);
		if (animId == mappedId) {
			startIndexedAnimation(i);
			break;
		}
	}
}

void Location::stopMappedAnimation(int index) {
	if (_mapEntry && index >= 0 && (uint)index < _mapEntry->animationMap.size()) {
		stopIndexedAnimation(_mapEntry->animationMap[index]);
	}
}

void Location::stopIndexedAnimation(int index) {
	if (index >= 0 && index < _locationAnimationCount && index < MAX_LOCATION_ANIMATIONS) {
		_animations[index].status = AnimationStatus::kCompleted;
	}
}

bool Location::isAnimationFinished(int index) const {
	if (!_mapEntry || index < 0 || (uint)index >= _mapEntry->animationMap.size())
		return true;
	int mappedIndex = _mapEntry->animationMap[index];
	return (mappedIndex >= 0 && mappedIndex < _locationAnimationCount && mappedIndex < MAX_LOCATION_ANIMATIONS &&
			_animations[mappedIndex].status == AnimationStatus::kCompleted);
}

bool Location::isIndexedAnimationFinished(int index) const {
	return (index >= 0 && index < _locationAnimationCount && index < MAX_LOCATION_ANIMATIONS &&
			(_animations[index].status == AnimationStatus::kCompleted || _animations[index].status == AnimationStatus::kNotStarted));
}

int Location::getAnimationFrame(int index) const {
	if (!_mapEntry || index < 0 || (uint)index >= _mapEntry->animationMap.size())
		return -1;
	int mappedIndex = _mapEntry->animationMap[index];
	return (mappedIndex >= 0 && mappedIndex < _locationAnimationCount && mappedIndex < MAX_LOCATION_ANIMATIONS) ? _animations[mappedIndex].frameCounter : -1;
}

int Location::getIndexedAnimationFrame(int index) const {
	return (index >= 0 && index < _locationAnimationCount && index < MAX_LOCATION_ANIMATIONS) ? _animations[index].frameCounter : -1;
}

void Location::animate() {
	if (_loading || !_animData)
		return;

	uint32 now = g_system ? g_system->getMillis() : 0;

	for (int i = 0; i < _locationAnimationCount && i < MAX_LOCATION_ANIMATIONS; i++) {
		if (_animations[i].status != AnimationStatus::kRunning)
			continue;

		uint32 frameTimeDiff = now - _animations[i].frameTime;
		if (frameTimeDiff < _animations[i].frameDuration)
			continue;

		_animations[i].frameTime = now;
		uint32 pA = _animations[i].animDataOffset;

		bool frameEnd = false;
		int safety = 0;
		while (!frameEnd && pA + 4 <= _animDataSize && safety++ < 256) {
			int32 p1 = READ_LE_INT32(_animData + pA);
			pA += 4;

			if (p1 != -1) {
				switch (_animations[i].type) {
				case 1: {
					int32 objectId = _animations[i].objectId;
					int32 subObjectId = _animations[i].parameter;
					int32 newTexture = p1;

					if (objectId >= 0 && (uint32)objectId < _objects.size()) {
						LocationObject &lo = _objects[objectId];
						for (uint soi = 0; soi < lo.subObjects.size(); soi++) {
							LocationSubObject &sub = lo.subObjects[soi];
							if ((int32)(sub.id & 0xffff) == subObjectId) {
								if (!sub.sprites.empty() && sub.textureIndex >= 0 && (uint32)sub.textureIndex < _textureGroups.size() &&
									newTexture >= 0 && (uint32)newTexture < _textures.size()) {
									_textureGroups[sub.textureIndex].overrideTexture = _textures[newTexture];
								} else {
									sub.textureIndex = newTexture;
									_buffersDirty = true;
								}
								break;
							}
						}
					}

					if (pA + 4 <= _animDataSize) {
						_animations[i].frameDuration = (uint32)(READ_LE_INT32(_animData + pA) * TIMER_SCALE);
						pA += 4;
					}
					frameEnd = true;
					break;
				}
				case 2: {
					if (pA + 12 <= _animDataSize) {
						float x = (float)p1 / 65536.0f;
						float y = (float)READ_LE_INT32(_animData + pA) / 65536.0f;
						float z = (float)READ_LE_INT32(_animData + pA + 4) / 65536.0f;
						_animations[i].frameDuration = (uint32)(READ_LE_INT32(_animData + pA + 8) * TIMER_SCALE);
						pA += 12;

						int32 objId = _animations[i].objectId;
						if (objId >= 0 && (uint32)objId < _objects.size()) {
							_objects[objId].translation.x() += x;
							_objects[objId].translation.y() += y;
							_objects[objId].translation.z() += z;
							_buffersDirty = true;
							_pointingChanged = true;
						}
					}
					frameEnd = true;
					break;
				}
				case 3: {
					int32 objectToHide = _animations[i].parameter;
					int32 objectToShow = p1;
					if (objectToShow >= 0 && pA + 4 <= _animDataSize) {
						_animations[i].frameDuration = (uint32)(READ_LE_INT32(_animData + pA) * TIMER_SCALE);
						pA += 4;

						if (objectToHide >= 0) {
							changeVisibility(objectToHide, false, false);
						}
						changeVisibility(objectToShow, true, false);
						_animations[i].parameter = objectToShow;
						_pointingChanged = true;
					}
					frameEnd = true;
					break;
				}
				case 4: {
					switch (p1) {
					case 1: {
						if (pA + 12 <= _animDataSize) {
							int32 objectId = READ_LE_INT32(_animData + pA);
							int32 subObjectId = READ_LE_INT32(_animData + pA + 4);
							int32 newTexture = READ_LE_INT32(_animData + pA + 8);
							pA += 12;

							if (objectId >= 0 && (uint32)objectId < _objects.size()) {
								LocationObject &lo = _objects[objectId];
								for (uint ts = 0; ts < lo.subObjects.size(); ts++) {
									LocationSubObject &sub = lo.subObjects[ts];
									if ((int32)(sub.id & 0xffff) == subObjectId) {
										int32 origTex = sub.originalTextureIndex;
										if (origTex >= 0 && (uint32)origTex < _textureGroups.size()) {
											if (newTexture != origTex && newTexture >= 0 && (uint32)newTexture < _textures.size()) {
												_textureGroups[origTex].overrideTexture = _textures[newTexture];
											} else {
												_textureGroups[origTex].overrideTexture = nullptr;
											}
										}
									}
								}
							}
						}
						break;
					}
					case 2: {
						if (pA + 20 <= _animDataSize) {
							int32 objId = READ_LE_INT32(_animData + pA);
							float x = (float)READ_LE_INT32(_animData + pA + 4) / 65536.0f;
							float y = (float)READ_LE_INT32(_animData + pA + 8) / 65536.0f;
							float z = (float)READ_LE_INT32(_animData + pA + 12) / 65536.0f;
							int32 dur = READ_LE_INT32(_animData + pA + 16);
							pA += 20;

							_animations[i].frameDuration = (uint32)(dur * TIMER_SCALE);
							if (objId >= 0 && (uint32)objId < _objects.size()) {
								_objects[objId].translation.x() += x;
								_objects[objId].translation.y() += y;
								_objects[objId].translation.z() += z;
								_buffersDirty = true;
								_pointingChanged = true;
							}
						}
						frameEnd = true;
						break;
					}
					case 3: {
						if (pA + 8 <= _animDataSize) {
							int32 objectId = READ_LE_INT32(_animData + pA);
							int32 visibility = READ_LE_INT32(_animData + pA + 4);
							pA += 8;
							changeVisibility(objectId, visibility != 0, false);
							_pointingChanged = true;
						}
						break;
					}
					case 5: {
						if (pA + 12 <= _animDataSize) {
							int32 objectId = READ_LE_INT32(_animData + pA);
							int32 subObjectId = READ_LE_INT32(_animData + pA + 4);
							int32 visibility = READ_LE_INT32(_animData + pA + 8);
							pA += 12;
							changeVisibility((int)(0x80000000u | ((uint32)objectId << 16) | ((uint32)subObjectId & 0xffff)), visibility != 0, false);
							_pointingChanged = true;
						}
						break;
					}
					case 6: {
						if (pA + 4 <= _animDataSize) {
							int32 animix = READ_LE_INT32(_animData + pA);
							pA += 4;
							startIndexedAnimation(animix);
						}
						break;
					}
					case 7: {
						if (pA + 4 <= _animDataSize) {
							int32 animix = READ_LE_INT32(_animData + pA);
							pA += 4;
							startIndexedAnimation(animix);
							if (animix >= 0 && animix < MAX_LOCATION_ANIMATIONS) {
								_animations[animix].parentAnim = i;
							}
							_animations[i].status = AnimationStatus::kOnHold;
						}
						frameEnd = true;
						break;
					}
					case 8: {
						if (pA + 8 <= _animDataSize) {
							int32 pid = READ_LE_INT32(_animData + pA);
							int32 enabled = READ_LE_INT32(_animData + pA + 4);
							pA += 8;
							for (uint p = 0; p < _paths.size(); p++) {
								if (_paths[p].id == pid) {
									_paths[p].enabled = (enabled != 0);
									break;
								}
							}
						}
						break;
					}
					default:
						frameEnd = true;
						break;
					}
					break;
				}
				case 14: {
					if (pA + 8 <= _animDataSize) {
						int32 animTex = p1;
						int32 baseTex = READ_LE_INT32(_animData + pA);
						int32 duration = READ_LE_INT32(_animData + pA + 4);
						if (duration == 0)
							duration = 3;
						pA -= 4;

						if (animTex >= 0 && (uint32)animTex < _textureGroups.size() &&
							baseTex >= 0 && (uint32)baseTex < _textureGroups.size()) {
							TextureGroup &animGroup = _textureGroups[animTex];
							if (!animGroup.animatedTextures.empty()) {
								if (animGroup.animatedTextureIndex < 0 || (uint32)animGroup.animatedTextureIndex >= animGroup.animatedTextures.size()) {
									animGroup.animatedTextureIndex = 0;
								}
								_textureGroups[baseTex].overrideTexture = animGroup.animatedTextures[animGroup.animatedTextureIndex++];
								if ((uint32)animGroup.animatedTextureIndex >= animGroup.animatedTextures.size()) {
									animGroup.animatedTextureIndex = 0;
								}
							}
						}
						_animations[i].frameDuration = (uint32)(duration * TIMER_SCALE);
					}
					frameEnd = true;
					break;
				}
				case 15: {
					int32 objectId = _animations[i].objectId;
					int32 subObjectId = _animations[i].parameter;
					int32 newTexture = p1;

					if (objectId >= 0 && (uint32)objectId < _objects.size()) {
						LocationObject &lo = _objects[objectId];
						for (uint ts = 0; ts < lo.subObjects.size(); ts++) {
							LocationSubObject &sub = lo.subObjects[ts];
							if ((int32)(sub.id & 0xffff) == subObjectId) {
								int32 origTex = sub.originalTextureIndex;
								if (origTex >= 0 && (uint32)origTex < _textureGroups.size() &&
									newTexture >= 0 && (uint32)newTexture < _textures.size()) {
									_textureGroups[origTex].overrideTexture = _textures[newTexture];
								}
							}
						}
					}

					_animations[i].frameDuration = _animations[i].constantFrameDuration;
					if (pA >= _animations[i].animDataOffsetEnd) {
						pA = _animations[i].animDataOffsetInit;
					}
					frameEnd = true;
					break;
				}
				default:
					frameEnd = true;
					break;
				}
			} else {
				if (pA + 4 > _animDataSize)
					break;
				int32 cmd = READ_LE_INT32(_animData + pA);
				pA += 4;
				switch (cmd) {
				case 0: {
					if (pA + 12 <= _animDataSize) {
						int32 p2 = READ_LE_INT32(_animData + pA);
						int32 p3 = READ_LE_INT32(_animData + pA + 4);
						int32 p4 = READ_LE_INT32(_animData + pA + 8);
						if (p3 == -1) {
							p3 = p2;
						}
						--p3;
						WRITE_LE_UINT32(_animData + pA + 4, (uint32)p3);
						pA += 12;
						if (p3 > 0) {
							pA -= (uint32)(p4 + 4);
						} else {
							WRITE_LE_UINT32(_animData + pA - 8, 0xffffffff);
						}
					} else {
						frameEnd = true;
					}
					break;
				}
				case 1: {
					if (pA + 4 <= _animDataSize) {
						int32 backOffset = READ_LE_INT32(_animData + pA);
						pA -= (uint32)backOffset;
					} else {
						frameEnd = true;
					}
					break;
				}
				case 2: {
					_animations[i].status = AnimationStatus::kCompleted;
					if (_animations[i].parentAnim >= 0 && _animations[i].parentAnim < MAX_LOCATION_ANIMATIONS) {
						_animations[_animations[i].parentAnim].status = AnimationStatus::kRunning;
					}
					frameEnd = true;
					break;
				}
				case 3: {
					if (pA + 4 <= _animDataSize) {
						_animations[i].frameDuration = (uint32)(READ_LE_INT32(_animData + pA) * TIMER_SCALE);
						pA += 4;
					}
					frameEnd = true;
					break;
				}
				default:
					frameEnd = true;
					break;
				}
			}
		}

		_animations[i].animDataOffset = pA;
		_animations[i].frameCounter++;
	}
}

bool Location::intersect(const Box &boundingBox, const Math::Vector3d &from, const Math::Vector3d &invDir) {
	float t1 = (boundingBox.x1 - from.x()) * invDir.x();
	float t2 = (boundingBox.x2 - from.x()) * invDir.x();
	float t3 = (boundingBox.y1 - from.y()) * invDir.y();
	float t4 = (boundingBox.y2 - from.y()) * invDir.y();
	float t5 = (boundingBox.z1 - from.z()) * invDir.z();
	float t6 = (boundingBox.z2 - from.z()) * invDir.z();

	float tmin = MAX(MAX(MIN(t1, t2), MIN(t3, t4)), MIN(t5, t6));
	float tmax = MIN(MIN(MAX(t1, t2), MAX(t3, t4)), MAX(t5, t6));

	return (tmax >= 0.0f && tmax >= tmin);
}

int Location::getPickObject(int &objectId, int &subObjectId) {
	if (_loading) {
		return -1;
	}

	_pointingChanged = false;
	objectId = -1;
	subObjectId = -1;

	Math::Vector3d ld(
		cos(_angle1) * sin(-_angle2),
		sin(_angle1),
		cos(_angle1) * cos(-_angle2));
	ld.normalize();

	Math::Vector3d invDir(
		(ld.x() != 0.0f) ? (1.0f / ld.x()) : 1e30f,
		(ld.y() != 0.0f) ? (1.0f / ld.y()) : 1e30f,
		(ld.z() != 0.0f) ? (1.0f / ld.z()) : 1e30f);

	float distance = 1000000.0f;

	for (int i = 0; i < (int)_objects.size(); i++) {
		if (!_objects[i].visible)
			continue;

		Math::Vector3d teye(
			-_x - _objects[i].translation.x(),
			-_y - _objects[i].translation.y(),
			-_z - _objects[i].translation.z());

		if (intersect(_objects[i].boundingBox, teye, invDir)) {
			for (int j = 0; j < (int)_objects[i].subObjects.size(); j++) {
				const LocationSubObject &subObj = _objects[i].subObjects[j];

				if (subObj.flags != 0 && subObj.visible) {
					if (intersect(subObj.boundingBox, teye, invDir)) {
						float td = 10000.0f;

						if (subObj.pointCount == 1) {
							float y1 = subObj.boundingBox.y1;
							float y2 = subObj.boundingBox.y2;
							float cx = (subObj.boundingBox.x1 + subObj.boundingBox.x2) / 2.0f;
							float cz = (subObj.boundingBox.z1 + subObj.boundingBox.z2) / 2.0f;
							float w = subObj.boundingBox.x2 - subObj.boundingBox.x1;

							float svx = teye.x() - cx;
							float svz = teye.z() - cz;
							float len = sqrt(svx * svx + svz * svz);
							if (len == 0.0f)
								len = 1.0f;
							svx /= len;
							svz /= len;

							float ppx = svz;
							float ppz = -svx;

							float ax = ppx * w / 2.0f;
							float az = ppz * w / 2.0f;

							Math::Vector3d v0(cx - ax, y1, cz - az);
							Math::Vector3d v1(cx + ax, y1, cz + az);
							Math::Vector3d v2(cx - ax, y2, cz - az);
							Math::Vector3d v3(cx + ax, y2, cz + az);

							if (MathUtils::intersectsTriangle(teye, ld, v0, v3, v2, td) ||
								MathUtils::intersectsTriangle(teye, ld, v0, v1, v3, td)) {
								if (td <= distance) {
									objectId = i;
									subObjectId = (int)subObj.id;
									distance = td;
								}
							}
						} else {
							for (uint t = 0; t < subObj.triangles.size(); t++) {
								const LocationTriangle &tri = subObj.triangles[t];
								if (MathUtils::intersectsTriangle(teye, ld, tri.p1.pos, tri.p2.pos, tri.p3.pos, td)) {
									if (td <= distance) {
										objectId = i;
										subObjectId = (int)subObj.id;
										distance = td;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	_hitObject = objectId;
	_hitSubObject = subObjectId;
	int prevObjectIndex = _objectIndex;
	_objectIndex = -1;

	if (_hitObject >= 0 && _mapEntry != nullptr) {
		for (uint ix = 0; ix < _mapEntry->objectMap.size(); ix++) {
			int oid = (int)_mapEntry->objectMap[ix];
			if (((_hitSubObject & 0x80000000) != 0 && _hitSubObject == oid) ||
				((_hitSubObject & 0x08000000) != 0 && oid == ((_hitSubObject >> 16) & 0xffff)) ||
				((_hitSubObject & 0x88000000) == 0 && _hitObject == oid)) {
				_objectIndex = (int)ix;
				break;
			}
		}
	}

	return _objectIndex;
}

void Location::setObjectVisibility(int objectId, bool visible) {
	if (!_mapEntry || objectId < 0 || (uint)objectId >= _mapEntry->objectMap.size())
		return;

	int id = (int)_mapEntry->objectMap[objectId];
	if ((uint32)id < 0x800u) {
		changeVisibility(id, visible, false);
	} else {
		for (uint i = 0; i < _objects.size(); i++) {
			for (uint j = 0; j < _objects[i].subObjects.size(); j++) {
				if ((int)((_objects[i].subObjects[j].id >> 16) & 0xffff) == id) {
					_objects[i].visible = visible;
				}
			}
		}
		// also handle 0x80000000 / 0x08000000 sub-object ids
		changeVisibility(id, visible, false);
	}
	_buffersDirty = true;
	_pointingChanged = true;
}

void Location::changeVisibility(int id, bool visible, bool setOnSubObjects) {
	if ((id & 0x80000000) != 0) {
		int objectId = (id >> 16) & 0x7fff;
		int subObjectId = id & 0xffff;
		if (objectId >= 0 && (uint)objectId < _objects.size()) {
			for (uint j = 0; j < _objects[objectId].subObjects.size(); j++) {
				if ((int)(_objects[objectId].subObjects[j].id & 0xffff) == subObjectId) {
					_objects[objectId].subObjects[j].visible = visible;
				}
			}
		}
	} else if ((id & 0x800) != 0) {
		for (uint i = 0; i < _objects.size(); i++) {
			for (uint j = 0; j < _objects[i].subObjects.size(); j++) {
				if ((int)((_objects[i].subObjects[j].id >> 16) & 0xffff) == id) {
					_objects[i].subObjects[j].visible = visible;
				}
			}
		}
	} else if (id >= 0 && (uint)id < _objects.size()) {
		_objects[id].visible = visible;
		if (setOnSubObjects) {
			for (uint j = 0; j < _objects[id].subObjects.size(); j++) {
				if ((_objects[id].subObjects[j].id & 0x80000000) == 0) {
					_objects[id].subObjects[j].visible = visible;
				}
			}
		}
	}

	_buffersDirty = true;
}

double Location::getPlayerDistanceFromPoint(double x, double z) {
	double dx = -_x - x;
	double dz = -_z - z;
	return dx * dx + dz * dz;
}

Math::Vector3d Location::getPlayerPosition() {
	return Math::Vector3d(-_x, _y_elevation + _y_player_adjustment, -_z);
}

Math::Vector3d Location::getUnadjustedPlayerPosition() {
	return Math::Vector3d(-_x, _y_elevation, -_z);
}

SpritePosInfo Location::getSpriteInfo(int index) {
	SpritePosInfo info;
	info.position = Math::Vector3d(0.0f, 0.0f, 0.0f);
	info.visible = false;

	if (index >= 0 && (uint)index < _objects.size()) {
		const LocationObject &obj = _objects[index];
		if (obj.subObjects.size() == 1 && !obj.subObjects[0].sprites.empty()) {
			info.position = obj.subObjects[0].sprites[0].pos + obj.translation;
			info.visible = obj.visible && obj.subObjects[0].visible;
		}
	}
	return info;
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

	// scan objects and sub-objects to determine which textures use alpha/sprites
	_transparentTextures.clear();
	_processedTextures.clear();
	for (uint32 tc = 0; tc < objectCount; tc++) {
		uint32 objectOffset = READ_LE_UINT32(p3d2 + 0x30 + tc * 4) + 0x30;
		if (objectOffset + 40 > streamSize)
			continue;
		uint32 subObjects = READ_LE_UINT32(p3d2 + objectOffset + 12);
		uint32 nextSubOffset = objectOffset + 40;
		for (uint32 ts = 0; ts < subObjects; ts++) {
			uint32 thisSubOffset = nextSubOffset;
			if (thisSubOffset + 0x28 > streamSize)
				break;
			nextSubOffset = READ_LE_UINT32(p3d2 + nextSubOffset) + 0x30;

			uint32 points = READ_LE_UINT32(p3d2 + thisSubOffset + 4);
			uint32 flags = READ_LE_UINT32(p3d2 + thisSubOffset + 8);
			int32 tex = (int32)READ_LE_UINT32(p3d2 + thisSubOffset + 0x24);

			if ((flags & SUBOBJECT_FLAGS_ALPHA) != 0 || points == 1) {
				_transparentTextures[tex] = true;
			}
			_processedTextures[tex] = true;

			if (nextSubOffset < thisSubOffset)
				break;
		}
	}

	// load textures now that transparency flags are known
	if (!loadTextures(archive)) {
		delete[] p3d2;
		return false;
	}

	// load vertices and apply ModifyLocationPoints z-fighting tweaks
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

	modifyLocationPoints(_currentFileName, objectCount);

	// parse objects and sub-objects
	_objects.resize(objectCount);
	int32 globalSubObjIdx = 0;

	for (uint32 i = 0; i < objectCount; i++) {
		// object offset table, add 0x30 to get real offset in entry
		uint32 objectOffset = READ_LE_UINT32(p3d2 + 0x30 + (i * 4)) + 0x30;

		// object flags (0x80000000 = object is hidden)
		_objects[i].flags = READ_LE_UINT32(p3d2 + objectOffset);
		_objects[i].visible = ((_objects[i].flags & OBJECT_FLAGS_HIDDEN) == 0);
		_objects[i].translation = Math::Vector3d(0.0f, 0.0f, 0.0f);

		uint32 unknownOffset = READ_LE_UINT32(p3d2 + objectOffset + 0x1c);

		float ominx = 10000.0f, omaxx = -10000.0f;
		float ominy = 10000.0f, omaxy = -10000.0f;
		float ominz = 10000.0f, omaxz = -10000.0f;

		// number of sub-objects
		uint32 subObjectsCount = READ_LE_UINT32(p3d2 + objectOffset + 12);
		_objects[i].subObjects.resize(subObjectsCount);

		uint32 nextSubOffset = objectOffset + 40;
		for (uint32 j = 0; j < subObjectsCount; j++) {
			float sminx = 10000.0f, smaxx = -10000.0f;
			float sminy = 10000.0f, smaxy = -10000.0f;
			float sminz = 10000.0f, smaxz = -10000.0f;

			uint32 thisSubOffset = nextSubOffset;
			// pointer to next sub-object, add 0x30
			nextSubOffset = READ_LE_UINT32(p3d2 + nextSubOffset) + 0x30;

			LocationSubObject &subObj = _objects[i].subObjects[j];
			subObj.subObjectIndex = globalSubObjIdx;
			subObj.triangles.clear();
			subObj.sprites.clear();

			if (thisSubOffset != unknownOffset) {
				uint32 points = READ_LE_UINT32(p3d2 + thisSubOffset + 4);
				subObj.pointCount = points;
			// sub-object flags (e.g., 0x00000002 = textured, 0x00000004 = sprite)
				subObj.flags = READ_LE_UINT32(p3d2 + thisSubOffset + 8);
				subObj.id = READ_LE_UINT32(p3d2 + thisSubOffset + 0x0C);
				subObj.visible = ((subObj.flags & SUBOBJECT_FLAGS_HIDDEN) == 0);
				int32 tex = (int32)READ_LE_UINT32(p3d2 + thisSubOffset + 0x24);
				if ((subObj.flags & SUBOBJECT_FLAGS_TEXTURED) == 0)
					tex = -1;
				subObj.textureIndex = tex;
				subObj.originalTextureIndex = tex;

				if (points == 1 && (subObj.flags & SUBOBJECT_FLAGS_SPRITE)) {
					uint32 subSprites = READ_LE_UINT32(p3d2 + thisSubOffset + 0x38);
					uint32 pointIndex = READ_LE_UINT32(p3d2 + thisSubOffset + 0x44 + (subSprites * 32)) >> 4;
					Math::Vector3d center = _vertices[pointIndex + objectCount];

					if (subSprites > 0) {
						float sww = READ_LE_INT32(p3d2 + thisSubOffset + 0x28) / 65536.0f;
						float swh = READ_LE_INT32(p3d2 + thisSubOffset + 0x2C) / 65536.0f;
						float ssw = READ_LE_INT32(p3d2 + thisSubOffset + 0x30) / 65536.0f;
						float ssh = READ_LE_INT32(p3d2 + thisSubOffset + 0x34) / 65536.0f;

						float tw = 1.0f, th = 1.0f;
						if (tex >= 0 && (uint32)tex < _textures.size() && _textures[tex]) {
							tw = (float)_textures[tex]->getWidth();
							th = (float)_textures[tex]->getHeight();
						}

						for (uint32 ss = 0; ss < subSprites; ss++) {
							float offsetX = READ_LE_INT32(p3d2 + thisSubOffset + 0x44 + ss * 32) / 65536.0f;
							float offsetY = READ_LE_INT32(p3d2 + thisSubOffset + 0x48 + ss * 32) / 65536.0f;
							float texX1 = READ_LE_INT32(p3d2 + thisSubOffset + 0x54 + ss * 32) / 65536.0f;
							float texY1 = READ_LE_INT32(p3d2 + thisSubOffset + 0x58 + ss * 32) / 65536.0f;
							float texX2 = 1.0f + (READ_LE_INT32(p3d2 + thisSubOffset + 0x5C + ss * 32) / 65536.0f);
							float texY2 = 1.0f + (READ_LE_INT32(p3d2 + thisSubOffset + 0x60 + ss * 32) / 65536.0f);

							SpriteInfo si;
							si.textureIndex = tex;
							si.pos = center;
							si.ox = (sww * offsetX) / ssw;
							si.oy = (swh * offsetY) / ssh;
							si.w = (sww * (texX2 - texX1)) / ssw;
							si.h = (swh * (texY2 - texY1)) / ssh;
							si.u1 = texX1 / tw;
							si.v1 = texY1 / th;
							si.u2 = texX2 / tw;
							si.v2 = texY2 / th;
							si.objectIndex = (int)i;
							si.subObjectIndex = globalSubObjIdx;
							si.subObjectId = subObj.id;

							subObj.sprites.push_back(si);

							if ((center.x() - si.w / 2.0f) < sminx)
								sminx = center.x() - si.w / 2.0f;
							if ((center.x() + si.w / 2.0f) > smaxx)
								smaxx = center.x() + si.w / 2.0f;
							if ((center.y() - si.h) < sminy)
								sminy = center.y() - si.h;
							if (center.y() > smaxy)
								smaxy = center.y();
							if ((center.z() - si.w / 2.0f) < sminz)
								sminz = center.z() - si.w / 2.0f;
							if ((center.z() + si.w / 2.0f) > smaxz)
								smaxz = center.z() + si.w / 2.0f;
						}
					} else {
						sminx = smaxx = center.x();
						sminy = smaxy = center.y();
						sminz = smaxz = center.z();
					}
				} else if (points >= 3) {
					float tw = 1.0f, th = 1.0f;
					if (tex >= 0 && (uint32)tex < _textures.size() && _textures[tex]) {
						tw = (float)_textures[tex]->getWidth();
						th = (float)_textures[tex]->getHeight();
					}

					uint32 uvOffset = thisSubOffset + 0x28 + (points * 4);
					Common::Array<PolygonPoint> polyPoints(points);

					for (uint32 p = 0; p < points; p++) {
						uint32 pix = READ_LE_UINT32(p3d2 + thisSubOffset + 0x28 + (p * 4)) >> 4;
						float fu = READ_LE_INT32(p3d2 + uvOffset + (p * 8)) / 65536.0f;
						float fv = READ_LE_INT32(p3d2 + uvOffset + 4 + (p * 8)) / 65536.0f;

						polyPoints[p].pos = _vertices[pix + objectCount];

						if (subObj.flags & SUBOBJECT_FLAGS_SINGLE_COLOUR) {
							polyPoints[p].u = 0.0f;
							polyPoints[p].v = 0.0f;
						} else {
							polyPoints[p].u = CLIP(fu / tw, 0.0f, 1.0f);
							polyPoints[p].v = CLIP(fv / th, 0.0f, 1.0f);
						}

						if (polyPoints[p].pos.x() < sminx)
							sminx = polyPoints[p].pos.x();
						if (polyPoints[p].pos.x() > smaxx)
							smaxx = polyPoints[p].pos.x();
						if (polyPoints[p].pos.y() < sminy)
							sminy = polyPoints[p].pos.y();
						if (polyPoints[p].pos.y() > smaxy)
							smaxy = polyPoints[p].pos.y();
						if (polyPoints[p].pos.z() < sminz)
							sminz = polyPoints[p].pos.z();
						if (polyPoints[p].pos.z() > smaxz)
							smaxz = polyPoints[p].pos.z();
					}

					int startp = 0;
					int pointsLeft = (int)points;
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

						if (fabs(v1.x() - v2.x()) > 0.0001f ||
							fabs(v1.y() - v2.y()) > 0.0001f ||
							fabs(v1.z() - v2.z()) > 0.0001f) {

							subObj.triangles.push_back({p0, p2, p1});
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
			} else {
				subObj.pointCount = 0;
				subObj.flags = 0;
				subObj.id = 0;
				subObj.visible = false;
				subObj.textureIndex = -1;
				subObj.originalTextureIndex = -1;
			}

			subObj.boundingBox = {sminx, sminy, sminz, smaxx, smaxy, smaxz};

			if (sminx < ominx)
				ominx = sminx;
			if (sminy < ominy)
				ominy = sminy;
			if (sminz < ominz)
				ominz = sminz;
			if (smaxx > omaxx)
				omaxx = smaxx;
			if (smaxy > omaxy)
				omaxy = smaxy;
			if (smaxz > omaxz)
				omaxz = smaxz;

			globalSubObjIdx++;

			if (nextSubOffset < thisSubOffset)
				break;
		}

		_objects[i].boundingBox = {ominx, ominy, ominz, omaxx, omaxy, omaxz};
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
	palBytes[0] = 0;
	palBytes[1] = 0;
	palBytes[2] = 0;

	_textures.resize(texCount);
	_textureGroups.resize(texCount);

	Common::Array<uint32> sourceOffsets(texCount);
	Common::HashMap<int, bool> animatedTexMap;

	for (uint32 t = 0; t < texCount; t++) {
		_textureGroups[t].textureIndex = (int32)t;
		_textureGroups[t].overrideTexture = nullptr;
		_textureGroups[t].animatedTextureIndex = -1;
		_textureGroups[t].vbo = 0;
		_textureGroups[t].vertexCount = 0;
		_textureGroups[t].spriteVbo = 0;
		_textureGroups[t].spriteVertexCount = 0;

		bool isTransparent = (_transparentTextures.contains((int)t) || !_processedTextures.contains((int)t));
		_textureGroups[t].transparent = isTransparent;

		sourceOffsets[t] = texPtr;

		uint32 w = READ_LE_UINT32(tex + texPtr);
		uint32 h = READ_LE_UINT32(tex + texPtr + 4);
		uint32 subType = READ_LE_UINT32(tex + texPtr + 8);

		texPtr += 12 + (4 * h);
		byte *scanData = tex + texPtr;

		if ((subType & 2) == 2) {
			animatedTexMap[(int)t] = true;
			_textures[t] = nullptr;
			_textureGroups[t].rotated = false;
		} else {
			bool isRotated = ((subType & 1) == 1);
			_textureGroups[t].rotated = isRotated;
			_textures[t] = new Texture(w, h, scanData, rgbaPalette, isTransparent, isRotated);
		}

		texPtr += (w * h);
	}

	// build type 14 animated texture frame sequences
	if (_animData && _animDataSize >= 4) {
		for (int i = 0; i < _locationAnimationCount; i++) {
			if ((uint32)(8 + i * 8 + 4) > _animDataSize)
				break;
			uint32 offset = READ_LE_UINT32(_animData + 8 + i * 8);
			if (offset + 20 > _animDataSize)
				continue;
			uint32 type = READ_LE_UINT32(_animData + offset);
			if (type == 14) {
				int32 animated = READ_LE_INT32(_animData + offset + 12);
				int32 baseTexture = READ_LE_INT32(_animData + offset + 16);
				if (animated >= 0 && (uint32)animated < texCount &&
					baseTexture >= 0 && (uint32)baseTexture < texCount &&
					animatedTexMap.contains(animated)) {

					byte *pBaseHeader = tex + sourceOffsets[baseTexture];
					byte *pModHeader = tex + sourceOffsets[animated];

					uint32 w = READ_LE_UINT32(pBaseHeader + 0);
					uint32 h = READ_LE_UINT32(pBaseHeader + 4);
					uint32 mw = READ_LE_UINT32(pModHeader + 0);
					uint32 mh = READ_LE_UINT32(pModHeader + 4);

					byte *pBaseImageOrig = pBaseHeader + 12 + 4 * h;
					byte *pBaseWork = new byte[w * h];
					memcpy(pBaseWork, pBaseImageOrig, w * h);

					byte *pMod = pModHeader + 12 + 4 * mh;
					byte *pEnd = pMod + mw * mh;

					uint16 flags = READ_LE_UINT16(pMod + 6);
					int fbc = (flags & TEXTURE_FLAGS_LARGE) ? 4 : 2;

					pMod += 8;
					while (pMod + fbc <= pEnd) {
						uint32 destination = 0;
						int32 bytesInFrame = (fbc == 4) ? READ_LE_INT32(pMod) : READ_LE_UINT16(pMod);
						pMod += fbc;
						while (bytesInFrame > 0 && pMod < pEnd) {
							int b = *(pMod++);
							bytesInFrame--;
							if ((b & 0x80) != 0) {
								destination += (b & 0x7f);
							} else {
								if (destination + b <= w * h && pMod + b <= pEnd) {
									memcpy(pBaseWork + destination, pMod, b);
								}
								destination += b;
								pMod += b;
								bytesInFrame -= b;
							}
						}

						bool isTrans = _textureGroups[baseTexture].transparent;
						bool isRot = _textureGroups[baseTexture].rotated;
						Texture *frameTex = new Texture(w, h, pBaseWork, rgbaPalette, isTrans, isRot);
						_textureGroups[animated].animatedTextures.push_back(frameTex);
						_textureGroups[animated].transparent = isTrans;
					}

					_textureGroups[animated].animatedTextureIndex = 0;
					delete[] pBaseWork;
				}
			}
		}
	}

	delete[] texData.data;
	return true;
}

void Location::modifyLocationPoints(const Common::String &file, uint32 objectCount) {
	if (file == "ALLEY.AP") {
		modifyLocationPoints(objectCount, 238, 241, 0.0f, 0.1f, 0.0f);
		modifyLocationPoints(objectCount, 270, 273, 0.0f, 0.15f, 0.0f);
		modifyLocationPoints(objectCount, 1395, 1398, 0.0f, 0.1f, 0.0f);
	} else if (file == "ARBOR.AP") {
		modifyLocationPoints(objectCount, 1023, 1028, -0.1f, 0.0f, 0.0f);
	} else if (file == "AV.AP") {
		modifyLocationPoints(objectCount, 131, 134, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 203, 210, 0.0f, 0.0f, -0.01f);
	} else if (file == "BEDROOM.AP") {
		modifyLocationPoints(objectCount, 58, 61, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 3652, 3655, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 3837, 3840, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 4025, 4028, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 4210, 4213, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 4402, 4405, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 4591, 4594, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 4781, 4784, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 62, 69, 0.0f, 0.0f, -0.01f);
	} else if (file == "CASTLE.AP") {
		modifyLocationPoints(objectCount, 87, 90, -0.01f, 0.0f, 0.0f);
	} else if (file == "COLOFF.AP") {
		modifyLocationPoints(objectCount, 20, 23, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 28, 31, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 36, 39, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 251, 254, 0.0f, 0.0f, -0.15f);
		modifyLocationPoints(objectCount, 468, 571, 0.0f, 0.0f, -0.2f);
		modifyLocationPoints(objectCount, 2921, 2924, 0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 2943, 2950, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2951, 2954, 0.0f, 0.02f, 0.0f);
		modifyLocationPoints(objectCount, 2955, 2958, 0.0f, -0.1f, 0.0f);
	} else if (file == "COUNTESS.AP") {
		modifyLocationPoints(objectCount, 50, 53, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 58, 65, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 70, 73, 0.0f, 0.015f, 0.0f);
		modifyLocationPoints(objectCount, 82, 85, 0.0f, -0.02f, 0.0f);
		modifyLocationPoints(objectCount, 86, 89, 0.0f, -0.01f, 0.0f);
		modifyLocationPoints(objectCount, 591, 598, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 599, 667, 0.0f, 0.4f, 0.0f);
		modifyLocationPoints(objectCount, 668, 707, 0.0f, 0.1f, 0.0f);
	} else if (file == "DUBOIS.AP") {
		modifyLocationPoints(objectCount, 120, 131, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 132, 139, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 140, 147, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 148, 155, 0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 156, 159, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 160, 171, 0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 172, 187, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 192, 195, -0.005f, 0.0f, 0.005f);
		modifyLocationPoints(objectCount, 260, 299, 0.0f, 0.0f, 0.3f);
		modifyLocationPoints(objectCount, 475, 478, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 517, 520, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 589, 592, 0.0f, 0.01f, 0.0f);
	} else if (file == "GRSHALL.AP") {
		modifyLocationPoints(objectCount, 613, 616, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 625, 628, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 637, 640, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 649, 652, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 524, 531, 0.0f, 0.0f, 0.01f);
	} else if (file == "HALL.AP") {
		modifyLocationPoints(objectCount, 154, 157, -1.0f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 158, 161, 1.0f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 162, 167, 0.0f, 0.0f, 1.0f);
		modifyLocationPoints(objectCount, 260, 263, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 380, 383, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 1782, 1785, 0.0f, 0.4f, 0.0f);
		modifyLocationPoints(objectCount, 174, 175, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 178, 183, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 185, 186, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 189, 190, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 193, 194, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 197, 198, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 202, 203, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 206, 219, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 228, 239, 0.01f, 0.0f, 0.0f);
	} else if (file == "JACUZZI.AP") {
		modifyLocationPoints(objectCount, 139, 142, -4.0f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 101, 104, 0.0f, -0.49f, 0.0f);
		modifyLocationPoints(objectCount, 420, 423, 0.0f, -0.45f, 0.0f);
		modifyLocationPoints(objectCount, 118, 119, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 121, 122, 0.0f, 0.0f, -0.01f);
	} else if (file == "LIBRARY.AP") {
		modifyLocationPoints(objectCount, 2495, 2498, -0.1f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 2499, 2502, -0.1f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 104, 107, 0.0f, 0.0f, -0.1f);
	} else if (file == "MAINSTRT.AP") {
		modifyLocationPoints(objectCount, 2933, 2988, 0.0f, 0.6f, 0.0f);
		modifyLocationPoints(objectCount, 2792, 2847, 0.0f, 0.6f, 0.0f);
		modifyLocationPoints(objectCount, 3039, 3078, 0.0f, 0.6f, 0.0f);
		modifyLocationPoints(objectCount, 3124, 3167, 0.0f, 0.6f, 0.0f);
		modifyLocationPoints(objectCount, 3136, 3139, 0.0f, -0.005f, 0.0f);
		modifyLocationPoints(objectCount, 3164, 3167, 0.0f, 0.2f, 0.0f);
		modifyLocationPoints(objectCount, 3132, 3135, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 3047, 3050, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 3063, 3066, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2933, 2936, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2957, 2960, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2796, 2799, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2804, 2811, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2840, 2843, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 3192, 3195, 0.0f, -0.01f, 0.0f);
		modifyLocationPoints(objectCount, 1708, 1717, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2304, 2305, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 2315, 2315, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 2321, 2321, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 281, 281, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 318, 318, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 320, 320, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 2160, 2162, 0.0f, 0.1f, 0.0f);
		modifyLocationPoints(objectCount, 2163, 2165, 0.0f, 0.25f, 0.0f);
		modifyLocationPoints(objectCount, 645, 738, 0.0f, 0.1f, 0.0f);
		modifyLocationPoints(objectCount, 1367, 1370, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 2667, 2670, 0.0f, 0.0f, 1.5f);
	} else if (file == "MOONHALL.AP") {
		modifyLocationPoints(objectCount, 1033, 1036, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 1045, 1048, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 1057, 1060, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 1069, 1072, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 1097, 1100, 0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 655, 655, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 662, 662, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 636, 636, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 675, 675, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 786, 786, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 789, 789, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 301, 301, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 304, 304, 0.0f, 0.01f, 0.0f);
	} else if (file == "OBSERVE.AP") {
		modifyLocationPoints(objectCount, 106, 113, -0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 881, 884, -0.02f, 0.0f, 0.0f);
	} else if (file == "RADIO.AP") {
		modifyLocationPoints(objectCount, 741, 746, 0.0f, 0.0f, -0.01f);
	} else if (file == "RUSTY.AP") {
		modifyLocationPoints(objectCount, 506, 506, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 508, 510, 0.0f, 0.0f, -0.01f);
	} else if (file == "SAFEROOM.AP") {
		modifyLocationPoints(objectCount, 104, 104, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 106, 106, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 109, 109, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 111, 111, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 418, 457, 0.0f, 0.1f, 0.0f);
		modifyLocationPoints(objectCount, 476, 476, 0.0f, 0.15f, 0.0f);
	} else if (file == "SECRET.AP") {
		modifyLocationPoints(objectCount, 81, 84, 0.0f, 0.0f, -0.01f);
	} else if (file == "STUDY.AP") {
		modifyLocationPoints(objectCount, 369, 372, 0.02f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 608, 611, -0.005f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 506, 509, 0.0f, 0.0f, -0.01f);
		modifyLocationPoints(objectCount, 61, 64, 0.01f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 157, 160, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 1112, 1115, -0.005f, 0.0f, 0.0f);
		modifyLocationPoints(objectCount, 2510, 2510, 0.0f, 0.0f, 0.022f);
		modifyLocationPoints(objectCount, 2512, 2512, 0.0f, 0.0f, 0.022f);
		modifyLocationPoints(objectCount, 2545, 2545, 0.0f, 0.0f, 0.022f);
		modifyLocationPoints(objectCount, 2548, 2548, 0.0f, 0.0f, 0.022f);
	} else if (file == "TEXOFF.AP") {
		modifyLocationPoints(objectCount, 68, 71, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 3410, 3413, 0.0f, 0.0f, 0.01f);
		modifyLocationPoints(objectCount, 2369, 2464, 0.0f, 0.235f, 0.0f);
		modifyLocationPoints(objectCount, 3886, 3889, 0.0f, 0.01f, 0.0f);
		modifyLocationPoints(objectCount, 776, 779, 0.0f, -0.33f, 0.0f);
		modifyLocationPoints(objectCount, 957, 960, 0.0f, -0.33f, 0.0f);
	}
}

void Location::modifyLocationPoints(uint32 objectCount, int startix, int endix, float x, float y, float z) {
	int modCount = endix - startix;
	uint32 baseIdx = (uint32)startix + objectCount;
	for (int i = 0; i <= modCount; i++) {
		uint32 idx = baseIdx + (uint32)i;
		if (idx < _vertices.size()) {
			_vertices[idx].x() += x;
			_vertices[idx].y() += y;
			_vertices[idx].z() += z;
		}
	}
}

Math::Vector4d Location::getTransparentColour(const Common::String &file, int objectId, int subObjectId) const {
	if (file == "ALLEY.AP") {
		return Math::Vector4d(1.0f, 1.0f, 1.0f, 0.4f);
	} else if (file == "CASTLE.AP") {
		return Math::Vector4d(0.0f, 0.0f, 0.6f, 0.4f);
	} else if (file == "JACUZZI.AP") {
		return Math::Vector4d(0.5f, 0.5f, 0.8f, 0.4f);
	} else if (file == "LIBRARY.AP") {
		return Math::Vector4d(1.0f, 1.0f, 1.0f, 0.1f);
	} else if (file == "RADIO.AP") {
		if (objectId == 22 && subObjectId == 16) {
			return Math::Vector4d(1.0f, 1.0f, 1.0f, 1.0f);
		}
		return Math::Vector4d(0.3f, 0.3f, 0.7f, 0.4f);
	} else if (file == "SCHANZEE.AP") {
		return Math::Vector4d(0.0f, 0.0f, 0.0f, 0.5f);
	} else if (file == "SECRET.AP") {
		return Math::Vector4d(1.0f, 1.0f, 1.0f, 0.1f);
	} else if (file == "STUDY.AP") {
		return Math::Vector4d(1.0f, 1.0f, 1.0f, 0.08f);
	}
	return Math::Vector4d(0.3f, 0.3f, 0.7f, 0.4f);
}

} // End of namespace Murphy3d
