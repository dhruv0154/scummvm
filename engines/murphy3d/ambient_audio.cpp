#include "murphy3d/ambient_audio.h"
#include "common/endian.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "murphy3d/archive.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/module_controller.h"

namespace Murphy3d {

Common::HashMap<int, AmbientAudio *> AmbientAudio::_sounds;
Common::List<AmbientAudio *> AmbientAudio::_soundsToDelete;

AmbientAudio::AmbientAudio(uint8 *data, int length) {
	_sourceVoice = nullptr;
	_pData = data;
	_length = length;
	_finished = true;
	_isLooping = false;
	_timeDisposed = 0;
}

AmbientAudio::~AmbientAudio() {
	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
		delete _sourceVoice;
		_sourceVoice = nullptr;
	}

	if (_pData != nullptr) {
		delete[] _pData;
		_pData = nullptr;
	}

	_length = 0;
}

bool AmbientAudio::loadEntryData(const Common::String &fileName, int itemIndex, uint8 *&outData, int &outLength) {
	outData = nullptr;
	outLength = 0;

	if (fileName.empty())
		return false;

	Archive arch;
	if (!arch.open(Common::Path(fileName))) {
		warning("AmbientAudio::loadEntryData: Failed to open archive '%s'", fileName.c_str());
		return false;
	}

	Common::SeekableReadStream *stream = arch.getStream(itemIndex);
	if (!stream) {
		warning("AmbientAudio::loadEntryData: Failed to open entry %d in '%s'", itemIndex, fileName.c_str());
		return false;
	}

	int len = stream->size();
	if (len > 0) {
		outData = new uint8[len];
		stream->read(outData, len);
		outLength = len;
	}

	delete stream;
	return (outData != nullptr);
}

void AmbientAudio::clear() {
	uint32 now = g_system ? g_system->getMillis() : 0;

	for (Common::HashMap<int, AmbientAudio *>::iterator it = _sounds.begin(); it != _sounds.end(); ++it) {
		if (it->_value != nullptr) {
			it->_value->stop();
			it->_value->_timeDisposed = now;
			_soundsToDelete.push_back(it->_value);
		}
	}

	_sounds.clear();
}

void AmbientAudio::destroy() {
	for (Common::HashMap<int, AmbientAudio *>::iterator it = _sounds.begin(); it != _sounds.end(); ++it) {
		if (it->_value != nullptr) {
			it->_value->stop();
			delete it->_value;
		}
	}
	_sounds.clear();

	for (Common::List<AmbientAudio *>::iterator it = _soundsToDelete.begin(); it != _soundsToDelete.end(); ++it) {
		delete *it;
	}
	_soundsToDelete.clear();
}

bool AmbientAudio::load(MapData *mapEntry, int entry) {
	if (_sounds.contains(entry)) {
		return true;
	}

	if (mapEntry != nullptr && entry >= 0 && entry < (int)mapEntry->environmentAudioMap.size()) {
		FileMap fm = mapEntry->environmentAudioMap[entry];
		Common::String file = GameController::getFileName(fm.file);
		uint8 *data = nullptr;
		int length = 0;
		if (loadEntryData(file, fm.entry, data, length)) {
			_sounds[entry] = new AmbientAudio(data, length);
			return true;
		}
	}

	return false;
}

void AmbientAudio::loop(MapData *mapEntry, int entry1, int entry2) {
	int entry = entry1 > 0 ? entry1 : entry2;
	entry = entry1 - 1;

	if (load(mapEntry, entry)) {
		_sounds[entry]->loop();
	}
}

void AmbientAudio::play(MapData *mapEntry, int entry, bool playAlways) {
	AmbientAudio *pSound = find(entry);
	if (pSound == nullptr || pSound->finished() || playAlways) {
		if (load(mapEntry, entry)) {
			_sounds[entry]->play();
		}
	}
}

void AmbientAudio::stop(int entry) {
	entry--;
	if (!_sounds.empty() && _sounds.contains(entry) && _sounds[entry] != nullptr) {
		_sounds[entry]->stop();
	}
}

void AmbientAudio::stopAll() {
	for (Common::HashMap<int, AmbientAudio *>::iterator it = _sounds.begin(); it != _sounds.end(); ++it) {
		if (it->_value != nullptr) {
			it->_value->stop();
		}
	}

	clear();
}

void AmbientAudio::loop() {
	play(true);
}

void AmbientAudio::play() {
	play(false);
}

void AmbientAudio::stop() {
	_isLooping = false;
	_finished = true;
	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
	}
}

bool AmbientAudio::finished() {
	if (_finished)
		return true;
	if (_sourceVoice != nullptr) {
		if (!_isLooping && _sourceVoice->getPendingBufferCount() == 0) {
			_finished = true;
			return true;
		}
	}
	return false;
}

void AmbientAudio::play(bool isLoop) {
	if (_pData == nullptr || _length <= 0x2c)
		return;

	_finished = false;
	_isLooping = isLoop;

	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
		delete _sourceVoice;
		_sourceVoice = nullptr;
	}

	AudioFormat fmt;
	fmt.channels = READ_LE_UINT16(_pData + 0x16);
	fmt.samplesPerSec = READ_LE_UINT32(_pData + 0x18);
	fmt.bitsPerSample = READ_LE_UINT16(_pData + 0x22);

	_sourceVoice = Sound::createAudioStream(fmt);
	if (_sourceVoice != nullptr) {
		uint32 payloadSize = _length - 0x2c;
		const uint8 *pPayload = _pData + 0x2c;

		_sourceVoice->submitBuffer(pPayload, payloadSize);
		if (_isLooping) {
			_sourceVoice->submitBuffer(pPayload, payloadSize);
		}
		_sourceVoice->start();
	}
}

void AmbientAudio::play(uint8 *pData) {
	if (pData == nullptr)
		return;

	_finished = false;
	_isLooping = false;

	if (_sourceVoice != nullptr) {
		_sourceVoice->stop();
		delete _sourceVoice;
		_sourceVoice = nullptr;
	}

	AudioFormat fmt;
	fmt.channels = READ_LE_UINT16(pData + 0x16);
	fmt.samplesPerSec = READ_LE_UINT32(pData + 0x18);
	fmt.bitsPerSample = READ_LE_UINT16(pData + 0x22);

	_sourceVoice = Sound::createAudioStream(fmt);
	if (_sourceVoice != nullptr) {
		int length = READ_LE_UINT32(pData + 0x28);
		if (length > 0x2c) {
			uint32 payloadSize = length - 0x2c;
			const uint8 *pPayload = pData + 0x2c;
			_sourceVoice->submitBuffer(pPayload, payloadSize);
			_sourceVoice->start();
		}
	}
}

void AmbientAudio::setVolume(int entry, float volume) {
	AmbientAudio *pAudio = find(entry);
	if (pAudio != nullptr) {
		pAudio->setVolume(volume);
	}
}

void AmbientAudio::setVolume(float volume) {
	if (_sourceVoice != nullptr) {
		_sourceVoice->setVolume(volume);
	}
}

AmbientAudio *AmbientAudio::find(int entry) {
	if (_sounds.contains(entry)) {
		return _sounds[entry];
	}
	return nullptr;
}

void AmbientAudio::setPan(int entry, float pan) {
	AmbientAudio *pAudio = find(entry);
	if (pAudio != nullptr) {
		pAudio->setPan(pan);
	}
}

void AmbientAudio::setPan(float pan) {
	(void)pan;
}

void AmbientAudio::gc() {
	uint32 now = g_system ? g_system->getMillis() : 0;

	for (Common::HashMap<int, AmbientAudio *>::iterator it = _sounds.begin(); it != _sounds.end(); ++it) {
		AmbientAudio *pSound = it->_value;
		if (pSound && pSound->_isLooping && pSound->_sourceVoice) {
			if (pSound->_sourceVoice->getPendingBufferCount() < 2 && pSound->_pData != nullptr && pSound->_length > 0x2c) {
				uint32 payloadSize = pSound->_length - 0x2c;
				pSound->_sourceVoice->submitBuffer(pSound->_pData + 0x2c, payloadSize);
			}
		}
	}

	Common::List<AmbientAudio *> deleteThese;
	for (Common::List<AmbientAudio *>::iterator it = _soundsToDelete.begin(); it != _soundsToDelete.end(); ++it) {
		if ((now - (*it)->_timeDisposed) >= 60000) {
			deleteThese.push_back(*it);
		}
	}

	for (Common::List<AmbientAudio *>::iterator it = deleteThese.begin(); it != deleteThese.end(); ++it) {
		_soundsToDelete.remove(*it);
		delete *it;
	}
}

} // End of namespace Murphy3d
