#ifndef MURPHY3D_ANIM_BASE_H
#define MURPHY3D_ANIM_BASE_H

#include "common/list.h"
#include "common/scummsys.h"
#include "graphics/opengl/system_headers.h"
#include "murphy3d/sound.h"
#include "murphy3d/texture.h"

namespace Murphy3d {

struct AudioBufferChunk {
	uint8 *pData;
	int size;
	int frame;
};

class AnimBase {
public:
	AnimBase();
	virtual ~AnimBase();

	virtual bool init(uint8 *pData, int length);

	void render();
	virtual bool update();

	virtual bool isWave() { return false; }
	virtual bool hasVideo() { return true; }
	virtual bool shouldClearDXBuffer() { return true; }

	virtual bool isDone() { return _done; }
	virtual void skip();
	int frame() const { return _frame; }

	int width() const { return _width; }
	int height() const { return _height; }

	void resize(int width, int height);

protected:
	Common::List<AudioBufferChunk> _audioBuffers;
	uint8 *_pInputBuffer;
	uint8 *_pVideoOutputBuffer;
	uint8 *_pRgbaBuffer;
	int _inputBufferLength;
	int _videoFramePointer;
	int _audioFramePointer;
	uint32 *_pPalette;

	int _width;
	int _height;
	int _rate;
	int _frameTime;
	int _frame;

	uint32 _lastFrameUpdate;

	virtual bool decodeFrame() { return false; }
	int _framePointer;

	virtual void createBuffers(int width, int height, int factor = 1);

	GLuint _vbo;
	Texture *_texture;

	AudioStream *_sourceVoice;
	int _remainingAudioLength;

	int _audioFramesQueued;
	int _audioFramesProcessed;
	int _videoFramesProcessed;

	int _screenWidth;
	int _screenHeight;

	bool _done;

	uint8 _colourTranslationTable[64];
};

} // End of namespace Murphy3d

#endif // MURPHY3D_ANIM_BASE_H
