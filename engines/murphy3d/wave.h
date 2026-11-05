#ifndef MURPHY3D_WAVE_H
#define MURPHY3D_WAVE_H

#include "murphy3d/anim_base.h"

namespace Murphy3d {

class Wave : public AnimBase {
public:
	Wave();
	~Wave() override;

	bool init(uint8 *pData, int length) override;
	bool isWave() override { return true; }
	bool hasVideo() override { return false; }

protected:
	bool decodeFrame() override;

	uint32 _timeOfStart;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_WAVE_H
