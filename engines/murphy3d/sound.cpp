#include "murphy3d/sound.h"
#include "audio/decoders/raw.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/util.h"

namespace Murphy3d {

float Sound::_masterVolume = 1.0f;

AudioStream::AudioStream(const AudioFormat &format)
	: _playing(false), _volume(1.0f), _format(format), _rawFlags(0), _queuingStream(nullptr) {

	if (_format.bitsPerSample == 16) {
		_rawFlags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
	} else {
		_rawFlags |= Audio::FLAG_UNSIGNED;
	}

	if (_format.channels == 2) {
		_rawFlags |= Audio::FLAG_STEREO;
	}

	uint32 rate = (_format.samplesPerSec > 0) ? _format.samplesPerSec : 22050;
	_queuingStream = Audio::makeQueuingAudioStream(rate, (_rawFlags & Audio::FLAG_STEREO) != 0);
}

AudioStream::~AudioStream() {
	stop();
	delete _queuingStream;
	_queuingStream = nullptr;
}

void AudioStream::submitBuffer(const uint8 *data, uint32 size) {
	if (!data || size == 0 || !_queuingStream)
		return;

	byte *bufCopy = (byte *)malloc(size);
	if (!bufCopy)
		return;

	memcpy(bufCopy, data, size);
	_queuingStream->queueBuffer(bufCopy, size, DisposeAfterUse::YES, _rawFlags);
}

void AudioStream::start() {
	if (!_queuingStream || _playing)
		return;

	_playing = true;
	int vol = (int)CLIP(_volume * Audio::Mixer::kMaxChannelVolume, 0.0f, (float)Audio::Mixer::kMaxChannelVolume);

	g_system->getMixer()->playStream(
		Audio::Mixer::kSpeechSoundType,
		&_handle,
		_queuingStream,
		-1,
		vol,
		0,
		DisposeAfterUse::NO);
}

void AudioStream::stop() {
	if (_playing && g_system && g_system->getMixer()) {
		g_system->getMixer()->stopHandle(_handle);
	}
	_playing = false;
}

void AudioStream::setVolume(float volume) {
	_volume = CLIP(volume, 0.0f, 1.0f);
	if (_playing && g_system && g_system->getMixer()) {
		int vol = (int)(_volume * Audio::Mixer::kMaxChannelVolume);
		g_system->getMixer()->setChannelVolume(_handle, vol);
	}
}

int AudioStream::getPendingBufferCount() const {
	if (!_queuingStream)
		return 0;
	return _queuingStream->numQueuedStreams();
}

bool AudioStream::isPlaying() const {
	if (!_playing || !g_system || !g_system->getMixer())
		return false;
	return g_system->getMixer()->isSoundHandleActive(_handle) &&
		   (_queuingStream->numQueuedStreams() > 0 || !_queuingStream->endOfStream());
}

Sound::Sound() {
}

Sound::~Sound() {
	stop();
}

AudioStream *Sound::createAudioStream(const AudioFormat &format) {
	return new AudioStream(format);
}

void Sound::play(const uint8 *data, uint32 size) {
	stop();

	if (!data || size <= 64 || !g_system || !g_system->getMixer())
		return;

	uint32 pcmSize = size - 64;
	byte *pcmCopy = (byte *)malloc(pcmSize);
	if (!pcmCopy)
		return;

	memcpy(pcmCopy, data + 64, pcmSize);

	Audio::AudioStream *stream = Audio::makeRawStream(
		pcmCopy,
		pcmSize,
		44100,
		Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN,
		DisposeAfterUse::YES);

	int vol = (int)CLIP(_masterVolume * Audio::Mixer::kMaxChannelVolume, 0.0f, (float)Audio::Mixer::kMaxChannelVolume);
	g_system->getMixer()->playStream(Audio::Mixer::kSFXSoundType, &_handle, stream, -1, vol);
}

void Sound::stop() {
	if (g_system && g_system->getMixer()) {
		g_system->getMixer()->stopHandle(_handle);
	}
}

void AudioStream::finish() {
	if (_queuingStream) {
		_queuingStream->finish();
	}
}

void Sound::setVolume(float volume) {
	_masterVolume = CLIP(volume, 0.0f, 1.0f);
}

} // End of namespace Murphy3d
