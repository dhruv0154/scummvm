#include "murphy3d/animation_controller.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "murphy3d/archive.h"
#include "murphy3d/caption.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/ptf_decoder.h"
#include "murphy3d/wave.h"

namespace Murphy3d {

AnimBase *AnimationController::_anim = nullptr;
Text *AnimationController::_pCaption = nullptr;

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

void AnimationController::clear() {
	if (_anim != nullptr) {
		delete _anim;
		_anim = nullptr;
	}

	if (_pCaption != nullptr) {
		_pCaption->setText("");
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
	if (_anim == nullptr) {
		delete[] data;
	}

	return (_anim != nullptr);
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

	if (pAnim != nullptr) {
		if (_pCaption == nullptr) {
			init();
		}

		int currFrame = frame(pAnim);
		Caption *pC = Caption::getFrameCaption(currFrame);
		if (pC != nullptr && !pC->processed()) {
			if (pC->tex()) {
				_pCaption->setColours(_texCaptionColour1, _texCaptionColour2, _texCaptionColour3, _texCaptionColour4);
			} else {
				_pCaption->setColours(_otherCaptionColour1, _otherCaptionColour2, _otherCaptionColour3, _otherCaptionColour4);
			}
			_pCaption->setText(pC->text(), TextAlignment::kAlignJustify);

			pC->setProcessed(true);
		}

		updated = pAnim->update();
		if (render) {
			pAnim->render();
		}

		if (!pAnim->isDone() && _pCaption != nullptr) {
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
		return nullptr;
	}

	uint32 sig0 = READ_LE_UINT32(data);
	uint32 sig4 = READ_LE_UINT32(data + 4);

	if (sig4 == 0x4654502e) {
		pAnim = new PTFDecoder(factor);
	} else if (sig0 == 0x46464952) {
		pAnim = new Wave();
	} else if (sig0 == 0x2b434942) {
		warning("AnimationController::load: BIC stub (not yet implemented)");
	} else if ((sig0 + 0x30c) == (uint32)length) {
		warning("AnimationController::load: SilentBIC stub (not yet implemented)");
	}

	if (pAnim != nullptr) {
		if (!pAnim->init(data, length)) {
			delete pAnim;
			pAnim = nullptr;
		}
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
	if (_anim != nullptr && !_anim->isDone() && _pCaption != nullptr) {
		_pCaption->render(g_engine->_renderer, 0.0f, g_system->getHeight() - _pCaption->getHeight() - 10.0f, z);
	}
}

} // End of namespace Murphy3d
