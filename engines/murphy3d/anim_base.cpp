#include "murphy3d/anim_base.h"
#include "common/system.h"
#include "common/util.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/renderer.h"
#include "murphy3d/shader_structs.h"
#include <math.h>

namespace Murphy3d {

AnimBase::AnimBase() {
	_pInputBuffer = nullptr;
	_pVideoOutputBuffer = nullptr;
	_pRgbaBuffer = nullptr;
	_inputBufferLength = 0;
	_videoFramePointer = 0;
	_audioFramePointer = 0;
	_pPalette = nullptr;

	_width = 0;
	_height = 0;
	_rate = 0;
	_frameTime = 100;
	_lastFrameUpdate = 0;
	_frame = 0;

	_vbo = 0;
	_texture = nullptr;

	_sourceVoice = nullptr;
	_remainingAudioLength = 0;

	_audioFramesQueued = 0;
	_audioFramesProcessed = 0;
	_videoFramesProcessed = 0;

	_screenWidth = 640;
	_screenHeight = 480;

	_framePointer = 0;
	_done = false;

	for (int i = 0; i < 64; i++) {
		_colourTranslationTable[i] = (uint8)(4.04762 * i);
	}
}

AnimBase::~AnimBase() {
	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
		delete _sourceVoice;
		_sourceVoice = nullptr;
	}

	delete[] _pInputBuffer;
	_pInputBuffer = nullptr;

	delete[] _pPalette;
	_pPalette = nullptr;

	delete[] _pVideoOutputBuffer;
	_pVideoOutputBuffer = nullptr;

	delete[] _pRgbaBuffer;
	_pRgbaBuffer = nullptr;

	delete _texture;
	_texture = nullptr;

	if (_vbo != 0 && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_vbo);
		_vbo = 0;
	}
}

bool AnimBase::init(uint8 *pData, int length) {
	_pInputBuffer = pData;
	_inputBufferLength = length;
	_screenWidth = g_system ? g_system->getWidth() : 640;
	_screenHeight = g_system ? g_system->getHeight() : 480;
	_pPalette = new uint32[256];
	memset(_pPalette, 0, sizeof(uint32) * 256);

	return true;
}

void AnimBase::createBuffers(int width, int height, int factor) {
	if (width <= 0 || height <= 0 || !g_engine || !g_engine->_renderer)
		return;

	int bufferHeight = (((height + 7) / 8) * 8);

	delete[] _pVideoOutputBuffer;
	_pVideoOutputBuffer = new uint8[width * bufferHeight];
	memset(_pVideoOutputBuffer, 0, width * bufferHeight);

	delete[] _pRgbaBuffer;
	_pRgbaBuffer = new uint8[width * height * 4];
	memset(_pRgbaBuffer, 0, width * height * 4);

	delete _texture;
	_texture = new Texture(width, height, _pRgbaBuffer);

	float sx = (float)_screenWidth / (float)(width * factor);
	float sy = (float)_screenHeight / (float)(height * factor);
	float scale = MIN(sx, sy);
	float sw = width * scale;
	float sh = height * scale;
	float ox = (_screenWidth - sw);
	float oy = (_screenHeight - sh);

	float left = floorf(ox / 2.0f);
	float right = floorf(left + sw);
	float top = -floorf(oy / 2.0f);
	float bottom = floorf(top - sh);

	TEXTURED_VERTEX vertices[6] = {
		{left, top, -1.0f, 0.0f, 0.0f},
		{right, top, -1.0f, 1.0f, 0.0f},
		{right, bottom, -1.0f, 1.0f, 1.0f},

		{left, top, -1.0f, 0.0f, 0.0f},
		{right, bottom, -1.0f, 1.0f, 1.0f},
		{left, bottom, -1.0f, 0.0f, 1.0f}};

	if (_vbo != 0) {
		g_engine->_renderer->deleteBuffer(_vbo);
	}
	_vbo = g_engine->_renderer->createVertexBuffer(vertices, sizeof(vertices), true);
}

void AnimBase::render() {
	if (_vbo != 0 && _texture != nullptr && g_engine && g_engine->_renderer) {
		Math::Matrix4 wm;
		wm.setToIdentity();
		g_engine->_renderer->drawUITextured(_vbo, 6, _texture->getId(), wm);
	}
}

bool AnimBase::update() {
	bool updated = false;

	if (!_done) {
		if (_sourceVoice != nullptr && !_audioBuffers.empty()) {
			while (!_audioBuffers.empty()) {
				AudioBufferChunk ab = _audioBuffers.front();
				_audioBuffers.pop_front();
				_sourceVoice->submitBuffer(ab.pData, ab.size);
			}
		}

		uint32 tick = g_system->getMillis();
		if (_lastFrameUpdate == 0) {
			_lastFrameUpdate = tick - _frameTime;
		}
		uint32 diff = tick - _lastFrameUpdate;

		if (diff >= (uint32)_frameTime) {
			if (decodeFrame()) {
				_frame++;

				if (_texture != nullptr && _pVideoOutputBuffer != nullptr && _pRgbaBuffer != nullptr) {
					const byte *rawPal = (const byte *)_pPalette;
					for (int i = 0; i < _width * _height; i++) {
						byte idx = _pVideoOutputBuffer[i];
						_pRgbaBuffer[i * 4 + 0] = rawPal[idx * 4 + 0];
						_pRgbaBuffer[i * 4 + 1] = rawPal[idx * 4 + 1];
						_pRgbaBuffer[i * 4 + 2] = rawPal[idx * 4 + 2];
						_pRgbaBuffer[i * 4 + 3] = 255;
					}

					glBindTexture(GL_TEXTURE_2D, _texture->getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE, _pRgbaBuffer);
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			}

			_lastFrameUpdate += _frameTime;

			bool audioFinished = true;
			if (_sourceVoice != nullptr) {
				audioFinished = !_sourceVoice->isPlaying();
			}

			if (isWave()) {
				if (audioFinished && _audioFramesQueued > 0) {
					_done = true;
				}
			} else if (_framePointer >= _inputBufferLength && audioFinished) {
				_done = true;
			}

			updated = true;
		}
	}

	return updated;
}

void AnimBase::skip() {
	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
		_audioFramesProcessed = _audioFramesQueued;
	}

	_audioBuffers.clear();
	_done = true;
}

void AnimBase::resize(int width, int height) {
	if (width > 0 && height > 0) {
		_screenWidth = width;
		_screenHeight = height;
		if (_width > 0 && _height > 0) {
			createBuffers(_width, _height, 1);
		}
	}
}

} // End of namespace Murphy3d
