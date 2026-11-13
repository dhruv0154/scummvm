#ifndef MURPHY3D_BIC_H
#define MURPHY3D_BIC_H

#include "common/scummsys.h"
#include "murphy3d/anim_base.h"

namespace Murphy3d {

class BIC : public AnimBase {
public:
	BIC(int factor = 1);
	~BIC() override = default;

	bool init(uint8 *pData, int length) override;

protected:
	bool decodeFrame() override;
	int processBICFrame(int inPtr, int chunkSize);

	int _embeddedAudioSize;
	int _firstAudioFrame;
	int _chunkTest;
	int _factor;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_BIC_H
