#ifndef MURPHY3D_ANIM_BASE_H
#define MURPHY3D_ANIM_BASE_H

#include "common/scummsys.h"

namespace Murphy3d {

class AnimBase {
public:
	AnimBase();
	virtual ~AnimBase();

	virtual bool init(const byte *data, uint32 length);
	virtual bool update();
	virtual void render() {}

	virtual bool isWave() const { return false; }
	virtual bool hasVideo() const { return true; }
	virtual bool shouldClearBuffer() const { return true; }
	virtual bool isDone() const { return _done; }
	virtual void skip();

	int frame() const { return _frame; }
	int width() const { return _width; }
	int height() const { return _height; }

protected:
	byte *_pInputBuffer;
	uint32 _inputBufferLength;

	uint8 _colourTranslationTable[64];

	int _width;
	int _height;
	int _rate;
	int _frameTime;
	int _frame;
	uint32 _lastFrameUpdate;
	bool _done;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_ANIM_BASE_H
