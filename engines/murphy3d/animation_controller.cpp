#include "murphy3d/animation_controller.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/stream.h"
#include "murphy3d/archive.h"
#include "murphy3d/caption.h"
#include "murphy3d/wave.h"

namespace Murphy3d {

AnimBase *AnimationController::_anim = nullptr;

void AnimationController::init() {
	_anim = nullptr;
}

void AnimationController::clear() {
	if (_anim != nullptr) {
		delete _anim;
		_anim = nullptr;
	}
	Caption::clearDisplayCaptions();
}

bool AnimationController::load(const Common::String &fileName, int itemIndex, int factor) {
	if (_anim != nullptr) {
		delete _anim;
		_anim = nullptr;
	}

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

	_anim = load(data, len, factor);
	delete[] data;

	return (_anim != nullptr);
}

AnimBase *AnimationController::load(const byte *data, uint32 length, int factor) {
	if (!data || length < 8)
		return nullptr;

	uint32 sig0 = READ_LE_UINT32(data);
	AnimBase *pAnim = nullptr;

	if (sig0 == 0x46464952) {
		// "RIFF" -> Wave
		pAnim = new Wave();
	} else if ((sig0 + 0x30c) == length) {
		// SilentBIC
		pAnim = new SilentBICAnim(factor);
	} else if (sig0 == 0x20434942) {
		// "BIC " -> BIC with embedded audio
		pAnim = new BICAnim(factor);
	}

	if (pAnim != nullptr) {
		if (!pAnim->init(data, length)) {
			delete pAnim;
			pAnim = nullptr;
		}
	}

	return pAnim;
}

void AnimationController::updateAndRender() {
	updateAndRender(_anim, true);
}

void AnimationController::updateAndRender(AnimBase *pAnim, bool render) {
	if (pAnim != nullptr) {
		int currentFrame = frame(pAnim);
		Caption::updateAndRender(currentFrame);

		pAnim->update();
		if (render) {
			pAnim->render();
		}

		// When the audio/animation finishes, clear the caption immediately
		if (pAnim->isDone()) {
			Caption::clearDisplayCaptions();
		}
	}
}

void AnimationController::skip() {
	if (_anim != nullptr) {
		_anim->skip();
		delete _anim;
		_anim = nullptr;
	}
	Caption::clearDisplayCaptions();
}

} // End of namespace Murphy3d
