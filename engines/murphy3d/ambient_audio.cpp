#include "murphy3d/ambient_audio.h"
#include "audio/audiostream.h"
#include "audio/decoders/raw.h"
#include "common/endian.h"
#include "murphy3d/archive.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/murphy3d.h"

namespace Murphy3d {

Common::HashMap<int, AmbientAudio *> AmbientAudio::_sounds;

AmbientAudio::AmbientAudio(const byte *data, uint32 length)
	: _pData(nullptr), _length(length), _isLooping(false) {
	if (data && length > 0) {
		_pData = new byte[length];
		memcpy(_pData, data, length);
	}
}

AmbientAudio::~AmbientAudio() {
	stopInstance();
	delete[] _pData;
	_pData = nullptr;
}

void AmbientAudio::clear() {
	for (auto &it : _sounds) {
		delete it._value;
	}
	_sounds.clear();
}

AmbientAudio *AmbientAudio::load(MapData *mapEntry, int entry) {
	if (_sounds.contains(entry)) {
		return _sounds[entry];
	}

	if (!mapEntry || entry < 0 || (uint)entry >= mapEntry->environmentAudioMap.size())
		return nullptr;

	FileMap fm = mapEntry->environmentAudioMap[entry];
	Common::String fn = GameController::getFileName(fm.file);
	if (fn.empty())
		return nullptr;

	Archive arch;
	if (!arch.open(Common::Path(fn)))
		return nullptr;

	Common::SeekableReadStream *s = arch.getStream(fm.entry);
	if (!s)
		return nullptr;

	uint32 len = s->size();
	byte *buf = new byte[len];
	s->read(buf, len);
	delete s;

	AmbientAudio *snd = new AmbientAudio(buf, len);
	delete[] buf;

	_sounds[entry] = snd;
	return snd;
}

void AmbientAudio::play(MapData *mapEntry, int entry, bool playAlways) {
	AmbientAudio *snd = load(mapEntry, entry);
	if (snd) {
		if (playAlways || snd->finished()) {
			snd->playInstance(false);
		}
	}
}

void AmbientAudio::loop(MapData *mapEntry, int entry1, int entry2) {
	AmbientAudio *snd = load(mapEntry, entry1);
	if (snd && snd->finished()) {
		snd->playInstance(true);
	}
}

void AmbientAudio::stop(int entry) {
	if (_sounds.contains(entry) && _sounds[entry]) {
		_sounds[entry]->stopInstance();
	}
}

void AmbientAudio::stopAll() {
	for (auto &it : _sounds) {
		if (it._value)
			it._value->stopInstance();
	}
}

void AmbientAudio::playInstance(bool loop) {
	stopInstance();

	if (!_pData || _length <= 0x2C || !g_engine || !g_engine->_mixer)
		return;

	uint32 sig = READ_LE_UINT32(_pData);
	if (sig != 0x46464952) // "RIFF"
		return;

	uint16 channels = READ_LE_UINT16(_pData + 0x16);
	uint32 sampleRate = READ_LE_UINT32(_pData + 0x18);
	uint16 bitsPerSample = READ_LE_UINT16(_pData + 0x22);
	uint32 audioLength = READ_LE_UINT32(_pData + 0x28);

	if (0x2C + audioLength > _length)
		audioLength = _length - 0x2C;
	if (audioLength == 0 || sampleRate == 0 || channels == 0)
		return;

	byte *rawPcm = (byte *)malloc(audioLength);
	if (!rawPcm)
		return;
	memcpy(rawPcm, _pData + 0x2C, audioLength);

	byte flags = 0;
	if (bitsPerSample == 16)
		flags |= Audio::FLAG_16BITS | Audio::FLAG_LITTLE_ENDIAN;
	else
		flags |= Audio::FLAG_UNSIGNED;
	if (channels == 2)
		flags |= Audio::FLAG_STEREO;

	Audio::RewindableAudioStream *rawStream = Audio::makeRawStream(rawPcm, audioLength, sampleRate, flags, DisposeAfterUse::YES);
	Audio::AudioStream *playStream = loop ? Audio::makeLoopingAudioStream(rawStream, 0) : static_cast<Audio::AudioStream *>(rawStream);

	_isLooping = loop;
	g_engine->_mixer->playStream(Audio::Mixer::kSFXSoundType, &_soundHandle, playStream);
}

void AmbientAudio::stopInstance() {
	if (g_engine && g_engine->_mixer) {
		g_engine->_mixer->stopHandle(_soundHandle);
	}
	_isLooping = false;
}

bool AmbientAudio::finished() const {
	if (!g_engine || !g_engine->_mixer)
		return true;
	return !g_engine->_mixer->isSoundHandleActive(_soundHandle);
}

} // End of namespace Murphy3d
