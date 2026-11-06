#ifndef MURPHY3D_AMBIENT_AUDIO_H
#define MURPHY3D_AMBIENT_AUDIO_H

#include "common/hashmap.h"
#include "common/list.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/map.h"
#include "murphy3d/sound.h"

namespace Murphy3d {

class AmbientAudio {
public:
	AmbientAudio(uint8 *data, int length);
	AmbientAudio() {
		_pData = nullptr;
		_length = 0;
		_sourceVoice = nullptr;
		_finished = true;
		_isLooping = false;
		_timeDisposed = 0;
	}
	~AmbientAudio();

	static void clear();
	static void destroy();
	static void loop(MapData *mapEntry, int entry1, int entry2);
	static void play(MapData *mapEntry, int entry, bool playAlways);
	static void stop(int entry);
	static void stopAll();

	void loop();
	void play();
	void play(uint8 *pData);
	void stop();

	static void setVolume(int entry, float volume);
	void setVolume(float volume);

	static void setPan(int entry, float pan);
	void setPan(float pan);

	static void gc();

	uint32 _timeDisposed;

protected:
	static bool load(MapData *mapEntry, int entry);
	static bool loadEntryData(const Common::String &fileName, int itemIndex, uint8 *&outData, int &outLength);

	static AmbientAudio *find(int entry);
	bool finished();
	bool _finished;
	bool _isLooping;

	void play(bool isLoop);

	uint8 *_pData;
	int _length;
	AudioStream *_sourceVoice;

	static Common::HashMap<int, AmbientAudio *> _sounds;
	static Common::List<AmbientAudio *> _soundsToDelete;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_AMBIENT_AUDIO_H
