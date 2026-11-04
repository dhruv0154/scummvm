#include "murphy3d/anim_base.h"
#include "common/system.h"

namespace Murphy3d {

AnimBase::AnimBase()
	: _pInputBuffer(nullptr), _inputBufferLength(0),
	  _width(0), _height(0), _rate(0), _frameTime(100),
	  _frame(0), _lastFrameUpdate(0), _done(false) {
	for (int i = 0; i < 64; i++) {
		_colourTranslationTable[i] = static_cast<uint8>(4.04762 * i);
	}
}

AnimBase::~AnimBase() {
	delete[] _pInputBuffer;
	_pInputBuffer = nullptr;
}

bool AnimBase::init(const byte *data, uint32 length) {
	delete[] _pInputBuffer;
	_pInputBuffer = nullptr;
	if (data && length > 0) {
		_pInputBuffer = new byte[length];
		memcpy(_pInputBuffer, data, length);
	}
	_inputBufferLength = length;
	_frame = 0;
	_done = false;
	_lastFrameUpdate = g_system ? g_system->getMillis() : 0;
	return true;
}

bool AnimBase::update() {
	if (_done || !g_system)
		return false;

	uint32 now = g_system->getMillis();
	if (now - _lastFrameUpdate >= (uint32)_frameTime) {
		_frame++;
		_lastFrameUpdate = now;
		return true;
	}
	return false;
}

void AnimBase::skip() {
	_done = true;
}

} // End of namespace Murphy3d
