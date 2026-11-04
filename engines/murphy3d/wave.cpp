#include "murphy3d/wave.h"
#include "audio/decoders/raw.h"
#include "common/debug.h"
#include "common/endian.h"
#include "audio/audiostream.h"
#include "common/system.h"
#include "murphy3d/murphy3d.h"

namespace Murphy3d {

Wave::Wave() : AnimBase(), _timeOfStart(0), _durationMs(0) {
	_frameTime = 100; // 100ms per caption frame tick (10 FPS), matching original Wave.cpp
}

Wave::~Wave() {
	skip();
}

bool Wave::init(const byte *data, uint32 length) {
	AnimBase::init(data, length);

	if (!data || length < 0x2C) {
		_done = true;
		return false;
	}

	// Check "RIFF" signature (0x46464952 in little-endian)
	uint32 sig = READ_LE_UINT32(data);
	if (sig != 0x46464952) {
		warning("Wave::init: Invalid RIFF signature 0x%08x", sig);
		_done = true;
		return false;
	}

	uint16 channels = READ_LE_UINT16(data + 0x16);
	uint32 sampleRate = READ_LE_UINT32(data + 0x18);
	uint16 bitsPerSample = READ_LE_UINT16(data + 0x22);
	uint32 audioLength = READ_LE_UINT32(data + 0x28);

	if (0x2C + audioLength > length) {
		audioLength = length - 0x2C;
	}

	if (audioLength == 0 || sampleRate == 0 || channels == 0) {
		_done = true;
		return false;
	}

	uint32 bytesPerSec = sampleRate * channels * (bitsPerSample / 8);
	_durationMs = (bytesPerSec > 0) ? (uint32)(((uint64)audioLength * 1000) / bytesPerSec) : 0;
	_timeOfStart = g_system ? g_system->getMillis() : 0;

	if (g_engine && g_engine->_mixer) {
		byte *rawPcm = (byte *)malloc(audioLength);
		if (rawPcm) {
			memcpy(rawPcm, data + 0x2C, audioLength);

			byte flags = 0;
			if (bitsPerSample == 16)
				flags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
			else
				flags |= Audio::FLAG_UNSIGNED;

			if (channels == 2)
				flags |= Audio::FLAG_STEREO;
			Audio::AudioStream *stream = static_cast<Audio::AudioStream*>(Audio::makeRawStream(rawPcm, audioLength, sampleRate, flags, DisposeAfterUse::YES));
			g_engine->_mixer->playStream(Audio::Mixer::kSpeechSoundType, &_soundHandle, stream);
		}
	}

	return true;
}

bool Wave::update() {
	if (_done)
		return false;

	bool frameAdvanced = AnimBase::update();

	bool mixerActive = (g_engine && g_engine->_mixer) ? g_engine->_mixer->isSoundHandleActive(_soundHandle) : false;
	uint32 now = g_system ? g_system->getMillis() : 0;
	bool timeExpired = (_durationMs > 0 && (now - _timeOfStart) >= _durationMs);

	if (!mixerActive && timeExpired) {
		_done = true;
	} else if (!mixerActive && (now - _timeOfStart) > 200) {
		// Mixer finished playing the buffer
		_done = true;
	}

	return frameAdvanced;
}

void Wave::skip() {
	if (g_engine && g_engine->_mixer) {
		g_engine->_mixer->stopHandle(_soundHandle);
	}
	_done = true;
}

} // End of namespace Murphy3d
