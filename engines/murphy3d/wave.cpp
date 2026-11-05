#include "murphy3d/wave.h"
#include "common/endian.h"
#include "common/system.h"

namespace Murphy3d {

Wave::Wave() : _timeOfStart(0) {
}

Wave::~Wave() {
}

bool Wave::init(uint8 *pData, int length) {
	bool ret = AnimBase::init(pData, length);

	// Validate that the file is a WAVE
	if (ret && length >= 0x2c && READ_LE_UINT32(pData) == 0x46464952) {
		_remainingAudioLength = READ_LE_UINT32(_pInputBuffer + 0x28);
		_frameTime = 100; // Default to 10 frames per second
	}

	return ret;
}

bool Wave::decodeFrame() {
	if (_remainingAudioLength > 0) {
		if (_sourceVoice == nullptr) {
			AudioFormat fmt;
			fmt.channels = READ_LE_UINT16(_pInputBuffer + 0x16);
			fmt.samplesPerSec = READ_LE_UINT32(_pInputBuffer + 0x18);
			fmt.bitsPerSample = READ_LE_UINT16(_pInputBuffer + 0x22);

			_sourceVoice = Sound::createAudioStream(fmt);
		}

		if (_sourceVoice != nullptr) {
			_sourceVoice->submitBuffer(_pInputBuffer + 0x2c, _remainingAudioLength);
			_sourceVoice->start();

			_audioFramesQueued = 1;
			_remainingAudioLength = 0;

			_timeOfStart = g_system->getMillis();
		}
	}

	return true;
}

} // End of namespace Murphy3d
