#include "murphy3d/animation_controller.h"
#include "common/compression/access_lzw.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "murphy3d/archive.h"
#include "murphy3d/bic.h"
#include "murphy3d/caption.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/ptf_decoder.h"
#include "murphy3d/shader_structs.h"
#include "murphy3d/silent_bic.h"
#include "murphy3d/wave.h"

namespace Murphy3d {

AnimBase *AnimationController::_anim = nullptr;
Text *AnimationController::_pCaption = nullptr;
Texture *AnimationController::_bgTexture = nullptr;
GLuint AnimationController::_bgVbo = 0;

int AnimationController::_texCaptionColour1 = 0xff000000;
int AnimationController::_texCaptionColour2 = 0xff00c300;
int AnimationController::_texCaptionColour3 = 0xff24ff00;
int AnimationController::_texCaptionColour4 = 0xff000000;
int AnimationController::_otherCaptionColour1 = 0xff000000;
int AnimationController::_otherCaptionColour2 = 0xff0096ff;
int AnimationController::_otherCaptionColour3 = 0xff00cfff;
int AnimationController::_otherCaptionColour4 = 0xff000000;

void AnimationController::init() {
	if (_pCaption == nullptr) {
		_pCaption = new Text();
	}
}

void AnimationController::clearImage() {
	delete _bgTexture;
	_bgTexture = nullptr;

	if (_bgVbo != 0 && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_bgVbo);
		_bgVbo = 0;
	}
}

void AnimationController::clear() {
	if (_anim != nullptr) {
		delete _anim;
		_anim = nullptr;
	}

	if (_pCaption != nullptr) {
		_pCaption->setText("");
	}
}

void AnimationController::destroy() {
	clearImage();

	if (_anim != nullptr) {
		delete _anim;
		_anim = nullptr;
	}

	if (_pCaption != nullptr) {
		delete _pCaption;
		_pCaption = nullptr;
	}
}

bool AnimationController::load(const Common::String &fileName, int itemIndex) {
	clear();

	Archive arch;
	if (!arch.open(Common::Path(fileName))) {
		warning("AnimationController::load: Failed to open archive '%s'", fileName.c_str());
		return false;
	}

	Common::SeekableReadStream *stream = arch.getStream(itemIndex);
	if (!stream) {
		warning("AnimationController::load: Failed to open entry %d in '%s'", itemIndex, fileName.c_str());
		return false;
	}

	uint32 len = stream->size();
	byte *data = new byte[len];
	stream->read(data, len);
	delete stream;

	_anim = load(data, len);
	if (_anim != nullptr && _anim->hasVideo()) {
		clearImage();
	}
	return (_anim != nullptr);
}

bool AnimationController::loadImage(const Common::String &fileName, int itemIndex) {
	clearImage();

	Archive arch;
	if (!arch.open(Common::Path(fileName))) {
		warning("AnimationController::loadImage: Failed to open archive '%s'", fileName.c_str());
		return false;
	}

	Common::SeekableReadStream *stream = arch.getStream(itemIndex);
	if (!stream) {
		warning("AnimationController::loadImage: Failed to open entry %d in '%s'", itemIndex, fileName.c_str());
		return false;
	}

	uint32 len = stream->size();
	byte *data = new byte[len];
	stream->read(data, len);
	delete stream;

	if (len > 0x304 && data[0x300] == 'D' && data[0x301] == 'B' && data[0x302] == 'E' && data[0x303] == 1) {
		byte *imageData = nullptr;
		uint32 imageSize = Common::decompressAccessDBE(data + 0x300, &imageData);
		if (imageData != nullptr && imageSize > 0) {
			int w = 640;
			int h = 480;
			byte *rgba = new byte[w * h * 4];
			int pixels = MIN((int)imageSize, w * h);
			for (int i = 0; i < pixels; i++) {
				byte idx = imageData[i];
				rgba[i * 4 + 0] = (uint8)((data[idx * 3 + 0] * 255) / 63);
				rgba[i * 4 + 1] = (uint8)((data[idx * 3 + 1] * 255) / 63);
				rgba[i * 4 + 2] = (uint8)((data[idx * 3 + 2] * 255) / 63);
				rgba[i * 4 + 3] = 255;
			}
			for (int i = pixels; i < w * h; i++) {
				rgba[i * 4 + 0] = 0;
				rgba[i * 4 + 1] = 0;
				rgba[i * 4 + 2] = 0;
				rgba[i * 4 + 3] = 255;
			}

			_bgTexture = new Texture(w, h, rgba);
			delete[] rgba;

			if (g_engine && g_engine->_renderer) {
				float screenW = g_system ? (float)g_system->getWidth() : 640.0f;
				float screenH = g_system ? (float)g_system->getHeight() : 480.0f;
				TEXTURED_VERTEX vertices[6] = {
					{0.0f, 0.0f, -1.0f, 0.0f, 0.0f},
					{screenW, 0.0f, -1.0f, 1.0f, 0.0f},
					{screenW, -screenH, -1.0f, 1.0f, 1.0f},

					{0.0f, 0.0f, -1.0f, 0.0f, 0.0f},
					{screenW, -screenH, -1.0f, 1.0f, 1.0f},
					{0.0f, -screenH, -1.0f, 0.0f, 1.0f}};
				_bgVbo = g_engine->_renderer->createVertexBuffer(vertices, sizeof(vertices), false);
			}
		}
		delete[] imageData;
	}

	delete[] data;
	return (_bgTexture != nullptr);
}

bool AnimationController::skip() {
	if (_anim != nullptr && !_anim->isDone()) {
		_anim->skip();
		return true;
	}

	return false;
}

bool AnimationController::isWave() {
	return (_anim != nullptr && _anim->isWave());
}

bool AnimationController::hasAnim() {
	return (_anim != nullptr && !_anim->isDone());
}

bool AnimationController::noAnimOrWave() {
	return (_anim == nullptr || _anim->isDone() || _anim->isWave());
}

bool AnimationController::updateAndRender(bool render) {
	return updateAndRender(_anim, render);
}

bool AnimationController::updateAndRender(AnimBase *pAnim, bool render) {
	bool updated = false;

	if (render && _bgTexture != nullptr && _bgVbo != 0 && (pAnim == nullptr || !pAnim->hasVideo()) && g_engine && g_engine->_renderer) {
		Math::Matrix4 wm;
		wm.setToIdentity();
		g_engine->_renderer->drawUITextured(_bgVbo, 6, _bgTexture->getId(), wm);
	}

	if (pAnim != nullptr) {
		if (_pCaption == nullptr) {
			init();
		}

		int currFrame = frame(pAnim);
		Caption *pC = Caption::getFrameCaption(currFrame);
		while (pC != nullptr && !pC->processed()) {
			if (pC->tex()) {
				_pCaption->setColours(_texCaptionColour1, _texCaptionColour2, _texCaptionColour3, _texCaptionColour4);
			} else {
				_pCaption->setColours(_otherCaptionColour1, _otherCaptionColour2, _otherCaptionColour3, _otherCaptionColour4);
			}
			_pCaption->setText(pC->text(), TextAlignment::kAlignJustify);

			pC->setProcessed(true);
			pC = Caption::getFrameCaption(currFrame);
		}

		updated = pAnim->update();
		if (render) {
			pAnim->render();
		}

		if (!pAnim->isDone() && _pCaption != nullptr && g_engine && g_engine->_renderer) {
			_pCaption->render(g_engine->_renderer, 0.0f, g_system->getHeight() - _pCaption->getHeight() - 10.0f, -1.0f);
		}
	}

	return updated;
}

bool AnimationController::isDone() {
	return (_anim != nullptr && _anim->isDone());
}

bool AnimationController::animNotDoneOrCondition(bool condition) {
	return (_anim != nullptr && (!_anim->isDone() || condition));
}

bool AnimationController::noVideoAnim() {
	return (_anim == nullptr || !_anim->hasVideo() || _anim->isDone());
}

int AnimationController::frame() {
	return frame(_anim);
}

int AnimationController::frame(AnimBase *pAnim) {
	if (pAnim == nullptr)
		return 0;
	return pAnim->frame();
}

AnimBase *AnimationController::load(uint8 *data, int length, int factor) {
	AnimBase *pAnim = nullptr;

	if (data == nullptr || length < 8) {
		warning("[AnimationController Error] Invalid or corrupt binary data passed to loader.");
		delete[] data;
		return nullptr;
	}

	uint32 sig0 = READ_LE_UINT32(data);
	uint32 sig4 = READ_LE_UINT32(data + 4);

	if (sig4 == 0x4654502e) {
		pAnim = new PTFDecoder(factor);
	} else if (sig0 == 0x46464952) {
		pAnim = new Wave();
	} else if (sig0 == 0x2b434942) {
		pAnim = new BIC(factor);
	} else if ((sig0 + 0x30c) == (uint32)length) {
		pAnim = new SilentBIC(factor);
	}

	if (pAnim != nullptr) {
		if (!pAnim->init(data, length)) {
			delete pAnim;
			pAnim = nullptr;
		}
	} else {
		delete[] data;
	}

	return pAnim;
}

void AnimationController::resize(int width, int height) {
	if (_anim != nullptr) {
		_anim->resize(width, height);
	}
}

void AnimationController::setOutputBuffer(uint8 *pBuffer, int width, int height, int offsetX, int offsetY, int *pPalette, int minColAllowChange, int maxColAllowChange) {
	// Stub for H2O embedded animations
}

void AnimationController::setCaptionColours(int texColour1, int texColour2, int texColour3, int texColour4, int otherColour1, int otherColour2, int otherColour3, int otherColour4) {
	_texCaptionColour1 = texColour1;
	_texCaptionColour2 = texColour2;
	_texCaptionColour3 = texColour3;
	_texCaptionColour4 = texColour4;
	_otherCaptionColour1 = otherColour1;
	_otherCaptionColour2 = otherColour2;
	_otherCaptionColour3 = otherColour3;
	_otherCaptionColour4 = otherColour4;
}

void AnimationController::renderCaptions(float z) {
	if (_anim != nullptr && !_anim->isDone() && _pCaption != nullptr && g_engine && g_engine->_renderer) {
		_pCaption->render(g_engine->_renderer, 0.0f, g_system->getHeight() - _pCaption->getHeight() - 10.0f, z);
	}
}

} // End of namespace Murphy3d
