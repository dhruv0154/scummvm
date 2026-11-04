#ifndef MURPHY3D_ANIMATION_CONTROLLER_H
#define MURPHY3D_ANIMATION_CONTROLLER_H

#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/anim_base.h"
#include "murphy3d/animation_controller.h"
#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "audio/mixer.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/stream.h"
#include "common/system.h"
#include "murphy3d/archive.h"
#include "murphy3d/caption.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/wave.h"

namespace Murphy3d {

// SilentBIC: steps through 0x30C-header BIC frame chunks at 100ms/frame to time captions
class SilentBICAnim : public AnimBase {
public:
	SilentBICAnim(int factor) : _framePointer(0x30C) {
		_rate = (factor > 0) ? factor : 6;
		_frameTime = 100;
	}

	bool hasVideo() const override { return false; }

	bool init(const byte *data, uint32 length) override {
		AnimBase::init(data, length);
		if (!data || length < 0x30C) {
			_done = true;
			return false;
		}
		_width = READ_LE_UINT16(data + 6);
		_height = READ_LE_UINT16(data + 8);
		_framePointer = 0x30C;
		decodeFrame();
		return true;
	}

	bool update() override {
		if (_done)
			return false;
		if (AnimBase::update()) {
			decodeFrame();
			return true;
		}
		return false;
	}

private:
	uint32 _framePointer;

	void decodeFrame() {
		if (_framePointer + 4 > _inputBufferLength) {
			_done = true;
			return;
		}
		uint32 chunkSize = READ_LE_UINT32(_pInputBuffer + _framePointer);
		_framePointer += chunkSize + 4;
		if (_framePointer >= _inputBufferLength) {
			_done = true;
		}
	}
};

// BIC: parses embedded WAVEFORMATEX audio + frame chunks starting at 0x340
class BICAnim : public AnimBase {
public:
	BICAnim(int factor)
		: _framePointer(0x340), _embeddedAudioSize(0), _chunkTest(0),
		  _remainingAudioLength(0), _audioStream(nullptr) {
		_rate = (factor > 0) ? factor : 6;
		_frameTime = 100;
	}

	~BICAnim() override {
		skip();
	}

	bool hasVideo() const override { return false; }

	bool init(const byte *data, uint32 length) override {
		AnimBase::init(data, length);
		if (!data || length < 0x340) {
			_done = true;
			return false;
		}

		_width = READ_LE_UINT16(data + 0x0A);
		_height = READ_LE_UINT16(data + 0x0C);
		_embeddedAudioSize = READ_LE_UINT16(data + 0x10);
		int16 firstAudioFrame = (int16)READ_LE_UINT16(data + 0x12);
		_chunkTest = 10 - firstAudioFrame;
		_remainingAudioLength = READ_LE_UINT32(data + 0x3C);

		uint16 channels = READ_LE_UINT16(data + 0x2A);
		uint32 sampleRate = READ_LE_UINT32(data + 0x2C);
		uint16 bitsPerSample = READ_LE_UINT16(data + 0x36);

		_framePointer = 0x340;

		if (_remainingAudioLength > 0 && _embeddedAudioSize > 0 && sampleRate > 0 && channels > 0 && g_engine && g_engine->_mixer) {
			_flags = 0;
			if (bitsPerSample == 16)
				_flags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
			else
				_flags |= Audio::FLAG_UNSIGNED;
			if (channels == 2)
				_flags |= Audio::FLAG_STEREO;

			_audioStream = Audio::makeQueuingAudioStream(sampleRate, (channels == 2));

			if (_chunkTest > 0) {
				uint32 preload = (uint32)_chunkTest * _embeddedAudioSize;
				if (_framePointer + preload <= _inputBufferLength) {
					queueAudio(_pInputBuffer + _framePointer, preload);
					_framePointer += preload;
					_remainingAudioLength = (_remainingAudioLength > preload) ? (_remainingAudioLength - preload) : 0;
				}
			}

			g_engine->_mixer->playStream(Audio::Mixer::kSpeechSoundType, &_soundHandle, _audioStream);
		}

		decodeFrame();
		return true;
	}

	bool update() override {
		if (_done)
			return false;
		if (AnimBase::update()) {
			decodeFrame();
			return true;
		}
		return false;
	}

	void skip() override {
		if (g_engine && g_engine->_mixer) {
			g_engine->_mixer->stopHandle(_soundHandle);
		}
		_audioStream = nullptr;
		_done = true;
	}

private:
	uint32 _framePointer;
	uint16 _embeddedAudioSize;
	int _chunkTest;
	uint32 _remainingAudioLength;
	byte _flags = 0;
	Audio::QueuingAudioStream *_audioStream;
	Audio::SoundHandle _soundHandle;

	void queueAudio(const byte *src, uint32 len) {
		if (!_audioStream || len == 0)
			return;
		byte *buf = (byte *)malloc(len);
		if (buf) {
			memcpy(buf, src, len);
			_audioStream->queueBuffer(buf, len, DisposeAfterUse::YES, _flags);
		}
	}

	void decodeFrame() {
		if (_framePointer >= _inputBufferLength) {
			if (_audioStream) {
				_audioStream->finish();
			}
			if (!g_engine || !g_engine->_mixer || !g_engine->_mixer->isSoundHandleActive(_soundHandle)) {
				_done = true;
			}
			return;
		}

		if (_remainingAudioLength > 0 && _embeddedAudioSize > 0) {
			uint32 audioChunk = MIN<uint32>(_remainingAudioLength, _embeddedAudioSize);
			if (_framePointer + _embeddedAudioSize <= _inputBufferLength) {
				queueAudio(_pInputBuffer + _framePointer, audioChunk);
				_framePointer += _embeddedAudioSize;
				_remainingAudioLength -= audioChunk;
			}
		}

		if (_framePointer + 4 <= _inputBufferLength) {
			uint32 videoChunkSize = READ_LE_UINT32(_pInputBuffer + _framePointer);
			_framePointer += videoChunkSize + 4;
		} else {
			_framePointer = _inputBufferLength;
		}
	}
};

class AnimationController {
public:
	static void init();
	static void clear();
	static bool load(const Common::String &fileName, int itemIndex, int factor = 1);
	static AnimBase *load(const byte *data, uint32 length, int factor = 1);

	static void updateAndRender();
	static void updateAndRender(AnimBase *pAnim, bool render = true);
	static void skip();

	static bool exists() { return _anim != nullptr; }
	static bool hasAnim() { return _anim != nullptr && !_anim->isDone(); }
	static bool isDone() { return _anim == nullptr || _anim->isDone(); }
	static bool noAnimOrWave() { return _anim == nullptr || _anim->isDone() || _anim->isWave(); }
	static bool noVideoAnim() { return _anim == nullptr || !_anim->hasVideo() || _anim->isDone(); }

	static int frame() { return frame(_anim); }
	static int frame(AnimBase *pAnim) { return pAnim ? pAnim->frame() : 0; }

private:
	static AnimBase *_anim;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_ANIMATION_CONTROLLER_H
