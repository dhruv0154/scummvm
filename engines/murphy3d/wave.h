#ifndef MURPHY3D_WAVE_H
#define MURPHY3D_WAVE_H

#include "audio/mixer.h"
#include "murphy3d/anim_base.h"

namespace Murphy3d {

class Wave : public AnimBase {
public:
	Wave();
	~Wave() override;

	bool init(const byte *data, uint32 length) override;
	bool update() override;
	void skip() override;

	bool isWave() const override { return true; }
	bool hasVideo() const override { return false; }

private:
	Audio::SoundHandle _soundHandle;
	uint32 _timeOfStart;
	uint32 _durationMs;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_WAVE_H
