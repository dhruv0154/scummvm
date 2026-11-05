#ifndef MURPHY3D_SOUND_H
#define MURPHY3D_SOUND_H

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "common/scummsys.h"

namespace Murphy3d {

struct AudioFormat {
	uint16 channels;
	uint32 samplesPerSec;
	uint16 bitsPerSample;
};

class AudioStream {
public:
	AudioStream(const AudioFormat &format);
	~AudioStream();

	void submitBuffer(const uint8 *data, uint32 size);
	void start();
	void stop();
	void finish();
	void setVolume(float volume);

	int getPendingBufferCount() const;
	bool isPlaying() const;

private:
	bool _playing;
	float _volume;
	AudioFormat _format;
	byte _rawFlags;

	Audio::QueuingAudioStream *_queuingStream;
	Audio::SoundHandle _handle;
};

class Sound {
public:
	Sound();
	~Sound();

	static AudioStream *createAudioStream(const AudioFormat &format);

	void play(const uint8 *data, uint32 size);
	void stop();

	static void setVolume(float volume);

private:
	Audio::SoundHandle _handle;
	static float _masterVolume;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_SOUND_H
