#ifndef MURPHY3D_AMBIENT_AUDIO_H
#define MURPHY3D_AMBIENT_AUDIO_H

#include "audio/mixer.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "murphy3d/map.h"

namespace Murphy3d {

class AmbientAudio {
public:
	AmbientAudio(const byte *data, uint32 length);
	~AmbientAudio();

	static void clear();
	static void play(MapData *mapEntry, int entry, bool playAlways);
	static void loop(MapData *mapEntry, int entry1, int entry2);
	static void stop(int entry);
	static void stopAll();

	void playInstance(bool loop);
	void stopInstance();
	bool finished() const;

private:
	byte *_pData;
	uint32 _length;
	Audio::SoundHandle _soundHandle;
	bool _isLooping;

	static Common::HashMap<int, AmbientAudio *> _sounds;
	static AmbientAudio *load(MapData *mapEntry, int entry);
};

} // End of namespace Murphy3d

#endif // MURPHY3D_AMBIENT_AUDIO_H
